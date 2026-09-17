/*
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 *
 * Secure JVM: Memory Proxy
 * A virtual private proxy layer that intercepts and manages all native memory
 * and I/O calls from child processes (.exe, .bin, .dmg, JNI libraries).
 *
 * License: GPL-2.0
 */

#ifndef JVM_MEMORY_PROXY_HPP
#define JVM_MEMORY_PROXY_HPP

#include <cstdint>
#include <cstddef>
#include <atomic>

// ============================================================================
//  Resource Budget Configuration
// ============================================================================

struct MemoryProxyBudget {
    // RAM limits (bytes)
    uint64_t ram_soft;              // Default: 512 MB
    uint64_t ram_hard;              // Default: 2 GB

    // Disk I/O limits (bytes/sec)
    uint64_t disk_write_rate_soft;  // Default: 100 MB/s
    uint64_t disk_write_rate_hard;  // Default: 500 MB/s
    uint64_t disk_read_rate_soft;   // Default: 200 MB/s
    uint64_t disk_read_rate_hard;   // Default: 1 GB/s

    // File descriptor limits
    uint32_t fd_soft;               // Default: 256
    uint32_t fd_hard;               // Default: 1024

    // CPU time per window (seconds of CPU per window_seconds real time)
    uint32_t cpu_window_seconds;    // Default: 10
    uint32_t cpu_soft_seconds;      // Default: 8
    uint32_t cpu_hard_seconds;      // Default: 10

    // Thread/process limits
    uint32_t threads_soft;          // Default: 64
    uint32_t threads_hard;          // Default: 256
    uint32_t children_soft;         // Default: 8
    uint32_t children_hard;         // Default: 32
};

// Default budget values
static const MemoryProxyBudget MEMORY_PROXY_DEFAULT_BUDGET = {
    .ram_soft             = 512ULL * 1024 * 1024,          // 512 MB
    .ram_hard             = 2ULL * 1024 * 1024 * 1024,     // 2 GB
    .disk_write_rate_soft = 100ULL * 1024 * 1024,          // 100 MB/s
    .disk_write_rate_hard = 500ULL * 1024 * 1024,          // 500 MB/s
    .disk_read_rate_soft  = 200ULL * 1024 * 1024,          // 200 MB/s
    .disk_read_rate_hard  = 1ULL * 1024 * 1024 * 1024,     // 1 GB/s
    .fd_soft              = 256,
    .fd_hard              = 1024,
    .cpu_window_seconds   = 10,
    .cpu_soft_seconds     = 8,
    .cpu_hard_seconds     = 10,
    .threads_soft         = 64,
    .threads_hard         = 256,
    .children_soft        = 8,
    .children_hard        = 32,
};

// ============================================================================
//  Alert Severity
// ============================================================================

enum class ProxyAlertSeverity : uint8_t {
    INFO    = 0,   // Informational (allocation churn, etc.)
    WARN    = 1,   // Approaching limits
    ERROR   = 2,   // Hard limit breach, action taken
    FATAL   = 3,   // Process killed (fork bomb, runaway)
};

// ============================================================================
//  Alert Types
// ============================================================================

enum class ProxyAlertType : uint16_t {
    ALLOCATION_CHURN     = 1,   // >1000 malloc/free cycles/s same size
    MEMORY_LEAK          = 2,   // Monotonic growth without free >60s
    READ_AMPLIFICATION   = 3,   // Same region read >10x without cache
    WRITE_FLOODING       = 4,   // >50 MB/s sustained, <1 MB logical
    FORK_BOMB            = 5,   // >4 children spawned within 1s
    SPIN_WAIT            = 6,   // CPU 100% with zero I/O for >5s
    FD_LEAK              = 7,   // >50 fds opened without close in 10s
    SOFT_RAM_BREACH      = 8,   // RAM exceeded soft limit
    HARD_RAM_BREACH      = 9,   // RAM exceeded hard limit (denied)
    SOFT_CPU_BREACH      = 10,  // CPU exceeded soft window
    HARD_CPU_BREACH      = 11,  // CPU exceeded hard window (SIGSTOP)
    SOFT_DISK_BREACH     = 12,  // Disk I/O exceeded soft limit
    HARD_DISK_BREACH     = 13,  // Disk I/O exceeded hard limit (blocked)
    THREAD_LIMIT         = 14,  // Thread count exceeded
    CHILD_LIMIT          = 15,  // Child process count exceeded
};

// ============================================================================
//  Process Verdict
// ============================================================================

enum class ProxyVerdict : uint8_t {
    HEALTHY      = 0,   // All within soft limits
    CAUTION      = 1,   // Approaching or at soft limits
    OVERUSE      = 2,   // Exceeding soft limits (throttled)
    CRITICAL     = 3,   // At hard limits (actions being denied)
    TERMINATED   = 4,   // Process was killed (fork bomb, etc.)
};

// ============================================================================
//  Per-Process Telemetry
// ============================================================================

struct MemoryProxyTelemetry {
    // Identification
    pid_t   pid;
    pid_t   parent_java_pid;
    char    binary_path[256];
    uint64_t start_time_ns;        // monotonic clock
    uint64_t uptime_seconds;

    // Memory telemetry
    std::atomic<uint64_t> ram_allocated;
    uint64_t ram_peak;
    uint64_t ram_alloc_rate_avg;   // bytes/sec (rolling average)
    uint64_t ram_alloc_rate_peak;  // bytes/sec (peak observed)
    uint64_t ram_free_rate_avg;    // bytes/sec
    double   fragmentation_pct;    // external fragmentation estimate
    double   leak_risk;            // 0.0 = none, 1.0 = certain

    // Disk I/O telemetry
    uint64_t disk_read_rate_avg;   // bytes/sec
    uint64_t disk_read_rate_peak;
    uint64_t disk_write_rate_avg;
    uint64_t disk_write_rate_peak;
    std::atomic<uint32_t> open_fds;
    char     io_pattern[32];       // "sequential", "random", "mixed"

    // CPU telemetry
    uint64_t cpu_user_ns;
    uint64_t cpu_system_ns;
    double   cpu_percent_10s;      // CPU% over last 10s window
    std::atomic<uint32_t> thread_count;
    int      nice_value;

    // Alert history (ring buffer, last 64 alerts)
    uint32_t alert_count;

    // Overall verdict
    ProxyVerdict verdict;
};

// ============================================================================
//  Alert Record
// ============================================================================

struct ProxyAlert {
    uint64_t          timestamp_ns;    // monotonic offset from process start
    ProxyAlertSeverity severity;
    ProxyAlertType    type;
    char              message[128];
};

// ============================================================================
//  Memory Proxy API
// ============================================================================

class JvmMemoryProxy {
public:
    // Lifecycle
    static bool initialize();
    static void shutdown();
    static bool is_active();

    // Process management
    static int  wrap_child_process(pid_t child_pid, const char* binary_path,
                                   const MemoryProxyBudget* budget);
    static void unwrap_child_process(pid_t child_pid);

    // Budget management
    static void set_default_budget(const MemoryProxyBudget* budget);
    static const MemoryProxyBudget* get_budget_for(const char* binary_path);
    static void set_application_budget(const char* binary_path,
                                       const MemoryProxyBudget* budget);

    // Telemetry queries
    static const MemoryProxyTelemetry* get_telemetry(pid_t child_pid);
    static ProxyVerdict get_verdict(pid_t child_pid);
    static int get_alert_history(pid_t child_pid, ProxyAlert* out_alerts,
                                 int max_alerts);

    // Status output (writes to /proc/jvm-proxy/)
    static int write_status_proc(pid_t child_pid, char* buf, size_t buf_len);
    static int write_global_status_proc(char* buf, size_t buf_len);

    // Configuration (from jvm-config.xml)
    static bool load_config_from_xml(const char* xml_path);

    // Alert callbacks
    typedef void (*alert_callback_fn)(pid_t pid, const ProxyAlert* alert);
    static void register_alert_callback(alert_callback_fn fn);

private:
    // Internal shim control
    static int  prepare_preload_shim(pid_t child_pid);
    static int  install_seccomp_filter(pid_t child_pid);
    static void monitor_thread_fn(pid_t child_pid);

    // Naive pattern detection
    static void check_allocation_churn(pid_t pid);
    static void check_unbounded_growth(pid_t pid);
    static void check_read_amplification(pid_t pid);
    static void check_write_flooding(pid_t pid);
    static void check_fork_bomb(pid_t pid);
    static void check_spin_wait(pid_t pid);
    static void check_fd_leak(pid_t pid);

    // Alert dispatch
    static void emit_alert(pid_t pid, ProxyAlertSeverity severity,
                           ProxyAlertType type, const char* message);
};

// ============================================================================
//  CLI Mode Entry Point
//  Called when the JVM is invoked with -memory-guard flag.
//  The JVM acts as a resource governance shell for any native binary.
//
//  Usage: java -memory-guard [-Xguard:flags...] <native-binary> [args...]
// ============================================================================

extern "C" int jvm_memory_proxy_cli_main(int argc, char** argv);

#endif // JVM_MEMORY_PROXY_HPP
