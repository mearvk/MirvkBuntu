/*
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 *
 * Secure JVM: Memory Proxy — Linux Platform Interception
 * LD_PRELOAD shim management and seccomp-bpf filter installation.
 *
 * License: GPL-2.0
 */

#include "jvmMemoryProxy.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>

// ============================================================================
//  Shared Memory Layout (JVM ↔ Shim Communication)
// ============================================================================

// This structure lives in /dev/shm/jvm-proxy-<PID>
// The shim writes counters; the JVM monitor thread reads them.

#define PROXY_SHM_MAGIC 0x4A564D50  // "JVMP"
#define PROXY_SHM_VERSION 1

struct ProxySharedMemory {
    uint32_t magic;                    // PROXY_SHM_MAGIC
    uint32_t version;                  // PROXY_SHM_VERSION
    pid_t    child_pid;

    // Allocation counters (written by shim)
    volatile uint64_t total_alloc_bytes;
    volatile uint64_t total_free_bytes;
    volatile uint64_t alloc_count;
    volatile uint64_t free_count;
    volatile uint64_t current_allocated;
    volatile uint64_t peak_allocated;

    // Per-size-class churn tracking (shim writes, JVM reads)
    volatile uint32_t churn_8;         // malloc/free cycles for 8-byte blocks
    volatile uint32_t churn_16;
    volatile uint32_t churn_32;
    volatile uint32_t churn_64;
    volatile uint32_t churn_128;
    volatile uint32_t churn_256;
    volatile uint32_t churn_512;
    volatile uint32_t churn_1024;
    volatile uint32_t churn_4096;
    volatile uint32_t churn_large;     // > 4096

    // I/O counters (written by shim)
    volatile uint64_t total_read_bytes;
    volatile uint64_t total_write_bytes;
    volatile uint64_t read_syscall_count;
    volatile uint64_t write_syscall_count;

    // File descriptor tracking
    volatile uint32_t fds_opened;
    volatile uint32_t fds_closed;
    volatile uint32_t fds_current;

    // Process/thread tracking
    volatile uint32_t fork_count;
    volatile uint32_t thread_count;

    // Shim control flags (written by JVM, read by shim)
    volatile uint32_t deny_alloc;      // 1 = deny future malloc (ENOMEM)
    volatile uint32_t deny_fork;       // 1 = deny fork/clone (EAGAIN)
    volatile uint32_t deny_open;       // 1 = deny open (EMFILE)
    volatile uint32_t throttle_write;  // 1 = add 1ms delay to writes
    volatile int32_t  nice_adjustment; // Applied to all threads
};

// ============================================================================
//  Shared Memory Management
// ============================================================================

static char* shm_path_for_pid(pid_t pid, char* buf, size_t buf_len) {
    snprintf(buf, buf_len, "/jvm-proxy-%d", (int)pid);
    return buf;
}

// Create shared memory segment for a child process
static ProxySharedMemory* create_shm(pid_t child_pid) {
    char shm_name[64];
    shm_path_for_pid(child_pid, shm_name, sizeof(shm_name));

    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (fd < 0) {
        // Already exists — reopen
        fd = shm_open(shm_name, O_RDWR, 0600);
        if (fd < 0) return nullptr;
    }

    if (ftruncate(fd, sizeof(ProxySharedMemory)) != 0) {
        close(fd);
        shm_unlink(shm_name);
        return nullptr;
    }

    ProxySharedMemory* shm = (ProxySharedMemory*)mmap(
        NULL, sizeof(ProxySharedMemory),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shm == MAP_FAILED) {
        shm_unlink(shm_name);
        return nullptr;
    }

    // Initialize
    memset(shm, 0, sizeof(ProxySharedMemory));
    shm->magic = PROXY_SHM_MAGIC;
    shm->version = PROXY_SHM_VERSION;
    shm->child_pid = child_pid;

    return shm;
}

// Destroy shared memory segment
static void destroy_shm(pid_t child_pid) {
    char shm_name[64];
    shm_path_for_pid(child_pid, shm_name, sizeof(shm_name));
    shm_unlink(shm_name);
}

// ============================================================================
//  LD_PRELOAD Environment Setup
// ============================================================================

// Prepares the environment for a child process so that jvmMemoryProxy_shim.so
// is loaded before any other library. The shim intercepts libc allocation
// and I/O functions.

static int setup_preload_env(pid_t child_pid, const char* java_home) {
    // Build shim path: $JAVA_HOME/lib/jvmMemoryProxy_shim.so
    char shim_path[512];
    snprintf(shim_path, sizeof(shim_path), "%s/lib/jvmMemoryProxy_shim.so", java_home);

    // Verify shim exists
    struct stat st;
    if (stat(shim_path, &st) != 0) {
        fprintf(stderr, "[JVM Memory Proxy] Shim not found: %s\n", shim_path);
        return -1;
    }

    // The actual LD_PRELOAD injection happens in ProcessBuilder's fork/exec path.
    // This function validates the shim is present and creates shared memory.
    ProxySharedMemory* shm = create_shm(child_pid);
    if (!shm) {
        fprintf(stderr, "[JVM Memory Proxy] Failed to create shared memory for PID %d\n",
                (int)child_pid);
        return -1;
    }

    return 0;
}

// ============================================================================
//  seccomp-bpf Filter for Fork/Clone Notification
// ============================================================================

// This filter is applied AFTER the child is forked but BEFORE exec.
// It uses SECCOMP_RET_TRACE to notify the JVM (via ptrace) when the child
// attempts fork/clone, allowing the JVM to track grandchildren.
//
// For the Memory Proxy, the primary use is:
// 1. Detecting fork bombs (rapid fork/clone calls)
// 2. Tracking thread creation for the thread ceiling
// 3. Enforcing the children-hard limit

// BPF filter that allows everything but traces fork/clone/clone3
static struct sock_filter seccomp_filter_insns[] = {
    // Load syscall number
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),

    // Check for clone (56 on x86_64)
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 56, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

    // Check for fork (57 on x86_64)
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 57, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

    // Check for vfork (58 on x86_64)
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 58, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

    // Check for clone3 (435 on x86_64)
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 435, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

    // Allow everything else
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
};

static struct sock_fprog seccomp_filter_prog = {
    .len = sizeof(seccomp_filter_insns) / sizeof(seccomp_filter_insns[0]),
    .filter = seccomp_filter_insns,
};

// Install the seccomp filter in the child process context
// (Called after fork, before exec, in the child)
static int install_seccomp_in_child() {
    // Allow ourselves to be ptraced by the parent JVM
    if (prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0) != 0) {
        // Non-fatal: some kernels don't support this
    }

    // Set no-new-privs (required for unprivileged seccomp)
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        return -errno;
    }

    // Install the BPF filter
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &seccomp_filter_prog, 0, 0) != 0) {
        return -errno;
    }

    return 0;
}

// ============================================================================
//  ProcessBuilder Integration Point
// ============================================================================

// This function is called from os_linux.cpp when ProcessBuilder launches a child.
// It sets up:
//   1. Shared memory segment
//   2. LD_PRELOAD environment variable pointing to the shim
//   3. seccomp filter installation (in child, pre-exec)
//
// Returns the shim path to add to the child's environment, or NULL on failure.

extern "C" const char* jvm_memory_proxy_prepare_child_env(
    pid_t anticipated_pid,
    const char* binary_path,
    const char* java_home)
{
    static char preload_env[600];

    // Validate inputs
    if (!binary_path || !java_home) return nullptr;

    // Build LD_PRELOAD value
    snprintf(preload_env, sizeof(preload_env),
             "LD_PRELOAD=%s/lib/jvmMemoryProxy_shim.so", java_home);

    // Create shared memory (the monitor thread in jvmMemoryProxy.cpp will read it)
    if (setup_preload_env(anticipated_pid, java_home) != 0) {
        return nullptr;
    }

    return preload_env;
}

// Called in the child process after fork but before exec
extern "C" int jvm_memory_proxy_child_pre_exec() {
    return install_seccomp_in_child();
}

// Called when a child process exits (cleanup)
extern "C" void jvm_memory_proxy_child_cleanup(pid_t child_pid) {
    destroy_shm(child_pid);
}
