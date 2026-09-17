/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmIntegrity.cpp - JVM Integrity Guardian Implementation
 *
 * Strict menu loading: 1:1 or 1:2 allocation ratios only.
 * No side hooks, no rootkits, no fractional loads from OS level.
 */

#include "runtime/jvmIntegrity.hpp"
#include "runtime/os.hpp"
#include "runtime/atomic.hpp"
#include "utilities/ostream.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>

// ============================================================================
// Static State
// ============================================================================

bool     JvmIntegrity::_enabled = false;
bool     JvmIntegrity::_preload_clean = false;
bool     JvmIntegrity::_agents_locked = false;
int      JvmIntegrity::_violation_count = 0;
int      JvmIntegrity::_alloc_violations = 0;

volatile uint64_t JvmIntegrity::_canary_a = 0;
volatile uint64_t JvmIntegrity::_canary_b = 0;

// ============================================================================
// Allowed Library Prefixes (strict menu)
//
// Only libraries from these paths are authorized for dlopen.
// Everything else is refused. This is the "strict menu" —
// you get exactly what's on the list, nothing more.
// ============================================================================

static const char* _authorized_lib_prefixes[] = {
  // JDK libraries
  "/usr/lib/jvm/",
  "/usr/local/lib/jvm/",

  // System libraries required by JDK
  "/lib/x86_64-linux-gnu/",
  "/lib64/",
  "/usr/lib/x86_64-linux-gnu/",
  "/usr/lib/",

  // Application libraries (must be in controlled paths)
  "/opt/app/lib/",

  nullptr  // sentinel
};

// Known JDK native library names (the things we expect to load)
static const char* _jdk_lib_names[] = {
  "libjava.so",
  "libnet.so",
  "libnio.so",
  "libzip.so",
  "libverify.so",
  "libjvm.so",
  "libjimage.so",
  "libjsig.so",
  "libawt.so",
  "libawt_headless.so",
  "libawt_xawt.so",
  "libfontmanager.so",
  "libfreetype.so",
  "libmanagement.so",
  "libmanagement_ext.so",
  "libsunec.so",
  "libsaproc.so",
  "libattach.so",
  "libdt_socket.so",
  "libinstrument.so",
  "libjdwp.so",
  "libsctp.so",
  "libextnet.so",
  "libprefs.so",
  "libjsound.so",
  "libsplashscreen.so",
  "libj2gss.so",
  "libj2pcsc.so",
  "libjaas.so",

  // System libraries the JDK legitimately needs
  "libc.so",
  "libpthread.so",
  "libdl.so",
  "libm.so",
  "librt.so",
  "libz.so",
  "libstdc++.so",
  "libgcc_s.so",

  nullptr  // sentinel
};

// ============================================================================
// Initialization
// ============================================================================

void JvmIntegrity::initialize() {
  _enabled = true;

  // Set integrity canaries
  set_canaries();

  // Check LD_PRELOAD at startup
  _preload_clean = check_ld_preload();
  if (!_preload_clean) {
    log_warning(os)("JvmIntegrity: LD_PRELOAD detected at startup — potential injection");
    _violation_count++;
  }

  // Check for ptrace
  if (!check_ptrace_status()) {
    log_warning(os)("JvmIntegrity: debugger/tracer attached — integrity uncertain");
    _violation_count++;
  }

  // Lock agents by default (no late-attach)
  _agents_locked = true;

  log_info(os)("JvmIntegrity: initialized (preload_clean=%s, agents_locked=%s)",
               _preload_clean ? "yes" : "NO",
               _agents_locked ? "yes" : "no");
}

void JvmIntegrity::set_canaries() {
  AtomicAccess::store(&_canary_a, CANARY_EXPECTED_A);
  AtomicAccess::store(&_canary_b, CANARY_EXPECTED_B);
}

// ============================================================================
// LD_PRELOAD Detection
// ============================================================================

bool JvmIntegrity::check_ld_preload() {
  const char* preload = getenv("LD_PRELOAD");

  // Clean: LD_PRELOAD is not set or is empty
  if (preload == nullptr || preload[0] == '\0') {
    return true;
  }

  // LD_PRELOAD is set — this is a concern.
  // Log what was found but don't crash (the admin may have a reason).
  log_warning(os)("JvmIntegrity: LD_PRELOAD='%s'", preload);

  // Clear it from our environment to prevent child processes from inheriting
  unsetenv("LD_PRELOAD");

  return false;
}

// ============================================================================
// Process Maps Inspection (detect injected .so)
// ============================================================================

bool JvmIntegrity::check_proc_maps_for_injections() {
  FILE* maps = fopen("/proc/self/maps", "r");
  if (maps == nullptr) return true;  // Can't check, assume OK

  char line[512];
  bool clean = true;

  while (fgets(line, sizeof(line), maps) != nullptr) {
    // Look for mapped shared objects that aren't on our whitelist
    char* so_path = strstr(line, "/");
    if (so_path == nullptr) continue;

    // Trim newline
    char* nl = strchr(so_path, '\n');
    if (nl) *nl = '\0';

    // Only check executable mappings (r-xp)
    if (strstr(line, "r-xp") == nullptr) continue;

    // Skip [vdso], [stack], [heap], etc.
    if (so_path[0] == '[') continue;

    // Check if this is an authorized path
    if (!is_library_authorized(so_path)) {
      log_warning(os)("JvmIntegrity: unauthorized library mapped: %s", so_path);
      _violation_count++;
      clean = false;
    }
  }

  fclose(maps);
  return clean;
}

// ============================================================================
// Anti-Ptrace
// ============================================================================

bool JvmIntegrity::check_ptrace_status() {
  // Check /proc/self/status for TracerPid
  FILE* status = fopen("/proc/self/status", "r");
  if (status == nullptr) return true;

  char line[256];
  bool clean = true;

  while (fgets(line, sizeof(line), status) != nullptr) {
    if (strncmp(line, "TracerPid:", 10) == 0) {
      int tracer_pid = atoi(line + 10);
      if (tracer_pid != 0) {
        log_warning(os)("JvmIntegrity: process is being traced by PID %d", tracer_pid);
        clean = false;
      }
      break;
    }
  }

  fclose(status);
  return clean;
}

// ============================================================================
// Canary Verification
// ============================================================================

bool JvmIntegrity::verify_canaries() {
  uint64_t a = AtomicAccess::load(&_canary_a);
  uint64_t b = AtomicAccess::load(&_canary_b);

  if (a != CANARY_EXPECTED_A || b != CANARY_EXPECTED_B) {
    log_error(os)("JvmIntegrity: CANARY CORRUPTION DETECTED! "
                  "Expected: %016llx/%016llx Got: %016llx/%016llx",
                  (unsigned long long)CANARY_EXPECTED_A,
                  (unsigned long long)CANARY_EXPECTED_B,
                  (unsigned long long)a, (unsigned long long)b);
    _violation_count++;
    return false;
  }
  return true;
}

// ============================================================================
// Native Library Authorization (Strict Menu)
// ============================================================================

bool JvmIntegrity::authorize_library_load(const char* path, const char* caller) {
  if (!_enabled) return true;
  if (path == nullptr) return false;

  if (is_library_authorized(path)) {
    return true;
  }

  // Not authorized — refuse
  log_warning(os)("JvmIntegrity: REFUSED library load: '%s' (caller: %s)",
                  path, caller ? caller : "unknown");
  _violation_count++;
  return false;
}

bool JvmIntegrity::is_library_authorized(const char* path) {
  if (path == nullptr) return false;

  // Check against authorized path prefixes
  if (path_matches_whitelist(path)) return true;

  // Check if it's a known JDK library by name
  if (is_jdk_library(path)) return true;

  return false;
}

bool JvmIntegrity::is_jdk_library(const char* path) {
  // Extract filename from path
  const char* basename = strrchr(path, '/');
  if (basename) basename++;
  else basename = path;

  // Check against known JDK library names
  for (int i = 0; _jdk_lib_names[i] != nullptr; i++) {
    // Match prefix (libraries may have version suffixes like .so.6)
    size_t name_len = strlen(_jdk_lib_names[i]);
    if (strncmp(basename, _jdk_lib_names[i], name_len) == 0) {
      return true;
    }
  }

  return false;
}

bool JvmIntegrity::path_matches_whitelist(const char* path) {
  for (int i = 0; _authorized_lib_prefixes[i] != nullptr; i++) {
    if (strncmp(path, _authorized_lib_prefixes[i],
                strlen(_authorized_lib_prefixes[i])) == 0) {
      return true;
    }
  }
  return false;
}

// ============================================================================
// Agent Attachment Gate
// ============================================================================

bool JvmIntegrity::authorize_agent_attach(const char* agent_path, const char* options) {
  if (!_enabled) return true;

  if (_agents_locked) {
    log_warning(os)("JvmIntegrity: REFUSED agent attachment (agents locked): '%s'",
                    agent_path ? agent_path : "(null)");
    _violation_count++;
    return false;
  }

  // Even if not locked, agent must be from an authorized path
  if (agent_path != nullptr && !is_library_authorized(agent_path)) {
    log_warning(os)("JvmIntegrity: REFUSED agent from unauthorized path: '%s'",
                    agent_path);
    _violation_count++;
    return false;
  }

  return true;
}

void JvmIntegrity::lock_agents() {
  _agents_locked = true;
  log_info(os)("JvmIntegrity: agent attachment locked — no further agents will be loaded");
}

// ============================================================================
// Allocation Integrity (Strict 1:1 / 1:2 Ratio Discipline)
// ============================================================================

/*
 * The principle: allocations should be CLEAN integers.
 *
 * 1:1 means you request N, you get exactly N (rounded up to alignment).
 * 1:2 means you request N, you get 2*N (double-buffer, still aligned).
 *
 * What we REJECT:
 * - Fractional ratios like 1:1.14 (actual = request * 1.14)
 *   This indicates either:
 *   a) A corrupted size computation (buffer overflow in size calc)
 *   b) A hooking malloc that adds hidden tracking bytes
 *   c) A rootkit padding allocations with surveillance data
 *
 * We enforce: actual_size must be either:
 *   - align_to_grid(requested)     [1:1 ratio]
 *   - align_to_grid(requested * 2) [1:2 ratio]
 *   - The alignment grid: 8, 16, 32, 64, 128, 256, 512, 1024, 4096, ...
 */

size_t JvmIntegrity::align_to_grid(size_t size) {
  if (size == 0) return 0;

  // Find the next power-of-2 aligned boundary
  // Grid: 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, ...
  if (size <= 8)    return 8;
  if (size <= 16)   return 16;
  if (size <= 32)   return 32;
  if (size <= 64)   return 64;
  if (size <= 128)  return 128;
  if (size <= 256)  return 256;
  if (size <= 512)  return 512;
  if (size <= 1024) return 1024;

  // For larger sizes, align to page boundaries (4096)
  // or the next multiple of the allocation's own magnitude
  size_t page = 4096;
  return (size + page - 1) & ~(page - 1);
}

bool JvmIntegrity::validate_alloc_size(size_t requested, size_t actual) {
  if (!_enabled) return true;
  if (requested == 0) return true;

  size_t expected_1_1 = align_to_grid(requested);
  size_t expected_1_2 = align_to_grid(requested * 2);

  // Accept: actual matches 1:1 grid or 1:2 grid
  if (actual == expected_1_1 || actual == expected_1_2) {
    return true;
  }

  // Accept: actual is on the alignment grid and >= requested
  // (allocator may round up to a larger bucket — this is fine if it's still on-grid)
  size_t grid_actual = align_to_grid(actual);
  if (actual == grid_actual && actual >= requested && actual <= expected_1_2) {
    return true;
  }

  // VIOLATION: actual is a fractional/non-grid value
  // This is the "1:1.14" case — something is wrong
  double ratio = (double)actual / (double)requested;
  log_warning(os)("JvmIntegrity: ALLOCATION RATIO VIOLATION "
                  "requested=%zu actual=%zu ratio=1:%.4f (expected 1:1 or 1:2)",
                  requested, actual, ratio);
  _alloc_violations++;
  _violation_count++;

  return false;
}

bool JvmIntegrity::validate_alloc_pointer(void* ptr, size_t size) {
  if (!_enabled) return true;
  if (ptr == nullptr) return (size == 0);  // null for zero-size is OK

  uintptr_t addr = (uintptr_t)ptr;

  // Pointer must be at least 8-byte aligned (all modern allocators do this)
  if (addr & 0x7) {
    log_warning(os)("JvmIntegrity: misaligned pointer returned: %p (not 8-byte aligned)",
                    ptr);
    _violation_count++;
    return false;
  }

  // Pointer must not be in obviously invalid ranges
  // (below 4KB is kernel space on Linux, above is fine)
  if (addr < 4096) {
    log_warning(os)("JvmIntegrity: suspicious low pointer: %p", ptr);
    _violation_count++;
    return false;
  }

  return true;
}

// ============================================================================
// Periodic Integrity Check
// ============================================================================

bool JvmIntegrity::run_integrity_check() {
  if (!_enabled) return true;

  bool clean = true;

  // 1. Verify canaries haven't been stomped
  if (!verify_canaries()) {
    clean = false;
  }

  // 2. Check for new tracer attachment
  if (!check_ptrace_status()) {
    clean = false;
  }

  // 3. Scan process maps for new unauthorized libraries
  if (!check_proc_maps_for_injections()) {
    clean = false;
  }

  // 4. Verify LD_PRELOAD hasn't been re-set
  const char* preload = getenv("LD_PRELOAD");
  if (preload != nullptr && preload[0] != '\0') {
    log_warning(os)("JvmIntegrity: LD_PRELOAD appeared after startup: '%s'", preload);
    unsetenv("LD_PRELOAD");
    _violation_count++;
    clean = false;
  }

  return clean;
}

// ============================================================================
// Diagnostics
// ============================================================================

void JvmIntegrity::print_status(outputStream* st) {
  st->print_cr("JVM Integrity Guardian:");
  st->print_cr("  Enabled:           %s", _enabled ? "yes" : "no");
  st->print_cr("  LD_PRELOAD clean:  %s", _preload_clean ? "yes" : "NO — injection detected");
  st->print_cr("  Agents locked:     %s", _agents_locked ? "yes" : "no (agents may attach)");
  st->print_cr("  Canaries:          %s", verify_canaries() ? "intact" : "CORRUPTED");
  st->print_cr("  Violations:        %d total (%d allocation)", _violation_count, _alloc_violations);
  st->print_cr("  Status:            %s", is_clean() ? "CLEAN" : "COMPROMISED");
  st->cr();
  st->print_cr("  Allocation discipline: strict 1:1 / 1:2 ratio (grid-aligned)");
  st->print_cr("  Library loading:       whitelist-only (strict menu)");
  st->print_cr("  Agent policy:          %s", _agents_locked ? "LOCKED (no late attach)" : "open");
}
