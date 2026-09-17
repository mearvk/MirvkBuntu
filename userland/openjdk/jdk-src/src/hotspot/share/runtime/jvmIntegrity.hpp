/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmIntegrity.hpp - JVM Integrity Guardian
 *
 * Prevents OS-level side hooks, rootkits, and illegitimate dynamic loads
 * from compromising the JVM. Enforces strict 1:1 and 1:2 memory allocation
 * ratios (no fractional loading such as 1:1.14).
 *
 * THREAT MODEL:
 *   - LD_PRELOAD injection (libc interposition)
 *   - dlopen() of unauthorized shared objects
 *   - JVMTI agent attachment (post-startup instrumentation)
 *   - ptrace/process_vm_readv surveillance
 *   - Malloc hooking (corrupted allocator returns)
 *   - Fractional allocation sizes (non-aligned, indicates corruption)
 *
 * DEFENSES:
 *   1. LD_PRELOAD detection and rejection at startup
 *   2. Whitelist-only native library loading (strict menu)
 *   3. JVMTI agent attachment blocking (unless pre-authorized)
 *   4. Allocation alignment enforcement (power-of-2 multiples only)
 *   5. Malloc return validation (pointer alignment, page membership)
 *   6. Periodic self-integrity checks (canary values, GOT verification)
 *   7. Anti-ptrace (detect debugger attachment)
 *
 * ALLOCATION DISCIPLINE:
 *   All JVM allocations follow strict ratio rules:
 *   - 1:1 = request N bytes, get exactly N (aligned up to boundary)
 *   - 1:2 = request N bytes, get 2N (double-buffer safety)
 *   - Never 1:1.14 or any fractional multiplier (indicates corrupted size)
 *   - Alignment: 8, 16, 32, 64, 128, 256, 512, 1024, 4096 bytes only
 *   - Any allocation not on this grid is rejected as suspicious
 */

#ifndef SHARE_RUNTIME_JVMINTEGRITY_HPP
#define SHARE_RUNTIME_JVMINTEGRITY_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"

// Allowed allocation ratios (request:actual)
enum AllocRatio {
  ALLOC_RATIO_1_1 = 1,   // Exact (aligned up to boundary)
  ALLOC_RATIO_1_2 = 2    // Double-buffer (2x request, aligned)
};

// Native library authorization status
enum LibAuth {
  LIB_AUTH_SYSTEM   = 0,  // Part of JDK distribution
  LIB_AUTH_ALLOWED  = 1,  // Explicitly whitelisted by admin
  LIB_AUTH_DENIED   = 2,  // Not on whitelist — refused
  LIB_AUTH_UNKNOWN  = 3   // Not yet checked
};

class JvmIntegrity : public AllStatic {
private:
  static bool _enabled;
  static bool _preload_clean;          // LD_PRELOAD was absent at startup
  static bool _agents_locked;          // No new agent attachment allowed
  static int  _violation_count;        // Total violations detected
  static int  _alloc_violations;       // Fractional/misaligned allocs caught

  // Canary values for self-integrity
  static volatile uint64_t _canary_a;
  static volatile uint64_t _canary_b;
  static const uint64_t CANARY_EXPECTED_A = 0x47414C4143544943ULL; // "GALACTIC"
  static const uint64_t CANARY_EXPECTED_B = 0x4348455252594D56ULL; // "CHERRYMV"

  // Internal checks
  static bool check_ld_preload();
  static bool check_proc_maps_for_injections();
  static bool check_ptrace_status();
  static bool verify_canaries();
  static void set_canaries();

  // Library whitelist
  static bool is_library_authorized(const char* path);
  static bool is_jdk_library(const char* path);
  static bool path_matches_whitelist(const char* path);

public:
  // Initialization — call early in JVM startup (before main())
  static void initialize();

  // =========================================================================
  // NATIVE LIBRARY GATE
  // Called before any dlopen() — returns true if library is allowed
  // =========================================================================
  static bool authorize_library_load(const char* path, const char* caller);

  // =========================================================================
  // AGENT ATTACHMENT GATE
  // Called when JVMTI agent tries to attach — returns true if allowed
  // =========================================================================
  static bool authorize_agent_attach(const char* agent_path, const char* options);

  // Lock agents: after JVM startup, refuse all new agent attachments
  static void lock_agents();

  // =========================================================================
  // ALLOCATION INTEGRITY
  // Validates that allocations follow strict ratio discipline
  // =========================================================================

  // Validate requested size is clean (power-of-2 aligned, not fractional)
  static bool validate_alloc_size(size_t requested, size_t actual);

  // Align size to nearest valid boundary (8, 16, 32, ... 4096)
  static size_t align_to_grid(size_t size);

  // Validate returned pointer is legitimate
  static bool validate_alloc_pointer(void* ptr, size_t size);

  // =========================================================================
  // PERIODIC INTEGRITY CHECK
  // Called from a watchdog thread or safepoint
  // =========================================================================
  static bool run_integrity_check();

  // =========================================================================
  // DIAGNOSTICS
  // =========================================================================
  static void print_status(outputStream* st);
  static int  violation_count() { return _violation_count; }
  static bool is_clean()        { return _violation_count == 0 && _preload_clean; }
};

#endif // SHARE_RUNTIME_JVMINTEGRITY_HPP
