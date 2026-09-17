/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * classLoadGuard.cpp - Class Loading Safety Guardian Implementation
 *
 * Enforces quantity ceilings and quality grading on class loading.
 * Integrates with SystemDictionary::load_instance_class().
 */

#include "classfile/classLoadGuard.hpp"
#include "runtime/arguments.hpp"
#include "runtime/atomic.hpp"
#include "utilities/ostream.hpp"

#include <string.h>
#include <ctype.h>

// ============================================================================
// Static State
// ============================================================================

bool         ClassLoadGuard::_enabled = false;
int          ClassLoadGuard::_global_max_classes = -1;      // unlimited by default
int          ClassLoadGuard::_global_loaded_count = 0;
int          ClassLoadGuard::_grade_threshold = 3;          // grades below (7-3)=4 are restricted
GuardPolicy  ClassLoadGuard::_policy = GUARD_POLICY_WARN;
GradeStats   ClassLoadGuard::_grade_stats[CLASS_GRADE_COUNT] = {};
volatile int ClassLoadGuard::_lock = 0;

// ============================================================================
// Locking (simple spinlock for stats updates)
// ============================================================================

void ClassLoadGuard::lock() {
  while (AtomicAccess::cmpxchg(&_lock, 0, 1) != 0) {
    os::naked_short_sleep(0);
  }
}

void ClassLoadGuard::unlock() {
  AtomicAccess::store(&_lock, 0);
}

// ============================================================================
// Grade Names
// ============================================================================

static const char* _grade_names[CLASS_GRADE_COUNT] = {
  "Ungraded",    // 0
  "Substitute",  // 1
  "Gainer",      // 2
  "Inheritor",   // 3
  "Builder",     // 4
  "Archetype",   // 5
  "Manager",     // 6
  "Main"         // 7
};

const char* ClassLoadGuard::grade_name(ClassGrade g) {
  if ((int)g >= 0 && (int)g < CLASS_GRADE_COUNT) {
    return _grade_names[(int)g];
  }
  return "Unknown";
}

// ============================================================================
// Initialization
// ============================================================================

void ClassLoadGuard::initialize() {
  // Set defaults for each grade
  for (int i = 0; i < CLASS_GRADE_COUNT; i++) {
    _grade_stats[i].loaded_count = 0;
    _grade_stats[i].max_allowed  = -1;  // unlimited by default
    _grade_stats[i].refused_count = 0;
    _grade_stats[i].warned_count = 0;
  }

  // Sensible defaults when enabled:
  // Main: unlimited (it's the entry point)
  // Manager: 100 (shouldn't need hundreds of orchestrators)
  // Archetype: 200 (abstract bases / interfaces)
  // Builder: 150 (factories and constructors)
  // Inheritor: 500 (concrete implementations)
  // Gainer: 300 (caches, accumulators)
  // Substitute: 200 (proxies, wrappers)
  // Ungraded: 2000 (libraries, third-party code)
  _grade_stats[CLASS_GRADE_MAIN].max_allowed       = -1;
  _grade_stats[CLASS_GRADE_MANAGER].max_allowed    = 100;
  _grade_stats[CLASS_GRADE_ARCHETYPE].max_allowed  = 200;
  _grade_stats[CLASS_GRADE_BUILDER].max_allowed    = 150;
  _grade_stats[CLASS_GRADE_INHERITOR].max_allowed  = 500;
  _grade_stats[CLASS_GRADE_GAINER].max_allowed     = 300;
  _grade_stats[CLASS_GRADE_SUBSTITUTE].max_allowed = 200;
  _grade_stats[CLASS_GRADE_UNGRADED].max_allowed   = 2000;

  _global_max_classes = 5000;
  _enabled = true;

  log_info(class, load)("ClassLoadGuard initialized: global_max=%d, policy=%s, threshold=%d",
                        _global_max_classes,
                        _policy == GUARD_POLICY_HARD ? "HARD" :
                        _policy == GUARD_POLICY_SOFT ? "SOFT" : "WARN",
                        _grade_threshold);
}

// ============================================================================
// Grade Detection
// ============================================================================

/*
 * Detect class grade by:
 * 1. Check for @ClassGrade annotation in bytecode (if present)
 * 2. Fall back to name-based heuristic
 *
 * The annotation approach allows developers to explicitly declare intent.
 * The heuristic catches common naming patterns (Manager, Builder, etc.)
 */
ClassGrade ClassLoadGuard::detect_grade(const char* class_name,
                                        const unsigned char* bytecode, size_t len) {
  // Try annotation-based detection first
  if (bytecode != nullptr && len > 0) {
    ClassGrade annotated = grade_from_annotation(bytecode, len);
    if (annotated != CLASS_GRADE_UNGRADED) {
      return annotated;
    }
  }

  // Fall back to name heuristic
  return grade_from_name_heuristic(class_name);
}

/*
 * Scan bytecode constant pool for a ClassGrade annotation.
 * Looks for the string "Lcom/galacticcherry/ClassGrade;" or similar marker.
 *
 * This is a lightweight scan — not a full classfile parse.
 * It searches for the annotation descriptor string in the constant pool bytes.
 */
ClassGrade ClassLoadGuard::grade_from_annotation(const unsigned char* bytecode, size_t len) {
  // Look for our custom annotation marker in the constant pool
  // Format: "ClassGrade:N" where N is 0-7
  static const char marker[] = "ClassGrade:";
  size_t marker_len = sizeof(marker) - 1;

  if (len < 64) return CLASS_GRADE_UNGRADED;  // Too small to contain annotation

  // Scan through bytecode for the marker string
  for (size_t i = 0; i < len - marker_len - 1; i++) {
    if (memcmp(bytecode + i, marker, marker_len) == 0) {
      char grade_char = (char)bytecode[i + marker_len];
      if (grade_char >= '0' && grade_char <= '7') {
        return (ClassGrade)(grade_char - '0');
      }
    }
  }

  return CLASS_GRADE_UNGRADED;
}

/*
 * Heuristic grade detection based on class name patterns.
 * Uses suffix and keyword matching on the simple class name.
 */
ClassGrade ClassLoadGuard::grade_from_name_heuristic(const char* class_name) {
  if (class_name == nullptr) return CLASS_GRADE_UNGRADED;

  // Get the simple name (after last / or .)
  const char* simple = class_name;
  const char* p = class_name;
  while (*p) {
    if (*p == '/' || *p == '.') simple = p + 1;
    p++;
  }

  size_t name_len = strlen(simple);
  if (name_len == 0) return CLASS_GRADE_UNGRADED;

  // Manager patterns: *Manager, *Controller, *Orchestrator, *Coordinator, *Service
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Manager") == 0) return CLASS_GRADE_MANAGER;
  if (name_len > 10 && strcasecmp(simple + name_len - 10, "Controller") == 0) return CLASS_GRADE_MANAGER;
  if (name_len > 12 && strcasecmp(simple + name_len - 12, "Orchestrator") == 0) return CLASS_GRADE_MANAGER;
  if (name_len > 11 && strcasecmp(simple + name_len - 11, "Coordinator") == 0) return CLASS_GRADE_MANAGER;
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Service") == 0) return CLASS_GRADE_MANAGER;

  // Archetype patterns: Abstract*, *Interface, *Base, *Template
  if (strncasecmp(simple, "Abstract", 8) == 0) return CLASS_GRADE_ARCHETYPE;
  if (name_len > 4 && strcasecmp(simple + name_len - 4, "Base") == 0) return CLASS_GRADE_ARCHETYPE;
  if (name_len > 8 && strcasecmp(simple + name_len - 8, "Template") == 0) return CLASS_GRADE_ARCHETYPE;

  // Builder patterns: *Builder, *Factory, *Creator, *Producer, *Generator
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Builder") == 0) return CLASS_GRADE_BUILDER;
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Factory") == 0) return CLASS_GRADE_BUILDER;
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Creator") == 0) return CLASS_GRADE_BUILDER;
  if (name_len > 8 && strcasecmp(simple + name_len - 8, "Producer") == 0) return CLASS_GRADE_BUILDER;
  if (name_len > 9 && strcasecmp(simple + name_len - 9, "Generator") == 0) return CLASS_GRADE_BUILDER;

  // Inheritor patterns: *Impl, *Default*, *Concrete*, *Real*
  if (name_len > 4 && strcasecmp(simple + name_len - 4, "Impl") == 0) return CLASS_GRADE_INHERITOR;
  if (strncasecmp(simple, "Default", 7) == 0) return CLASS_GRADE_INHERITOR;
  if (strncasecmp(simple, "Concrete", 8) == 0) return CLASS_GRADE_INHERITOR;

  // Gainer patterns: *Cache, *Pool, *Accumulator, *Collector, *Registry, *Store, *Repository
  if (name_len > 5 && strcasecmp(simple + name_len - 5, "Cache") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 4 && strcasecmp(simple + name_len - 4, "Pool") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 11 && strcasecmp(simple + name_len - 11, "Accumulator") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 9 && strcasecmp(simple + name_len - 9, "Collector") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 8 && strcasecmp(simple + name_len - 8, "Registry") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 5 && strcasecmp(simple + name_len - 5, "Store") == 0) return CLASS_GRADE_GAINER;
  if (name_len > 10 && strcasecmp(simple + name_len - 10, "Repository") == 0) return CLASS_GRADE_GAINER;

  // Substitute patterns: *Proxy, *Adapter, *Decorator, *Wrapper, *Delegate, *Stub, *Mock
  if (name_len > 5 && strcasecmp(simple + name_len - 5, "Proxy") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Adapter") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 9 && strcasecmp(simple + name_len - 9, "Decorator") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 7 && strcasecmp(simple + name_len - 7, "Wrapper") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 8 && strcasecmp(simple + name_len - 8, "Delegate") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 4 && strcasecmp(simple + name_len - 4, "Stub") == 0) return CLASS_GRADE_SUBSTITUTE;
  if (name_len > 4 && strcasecmp(simple + name_len - 4, "Mock") == 0) return CLASS_GRADE_SUBSTITUTE;

  return CLASS_GRADE_UNGRADED;
}

// ============================================================================
// Quantity Check
// ============================================================================

bool ClassLoadGuard::check_quantity(ClassGrade grade, const char* class_name) {
  // Check global ceiling
  if (_global_max_classes > 0 && _global_loaded_count >= _global_max_classes) {
    log_violation(grade, class_name, "global class limit reached");
    return false;
  }

  // Check per-grade ceiling
  int grade_idx = (int)grade;
  if (_grade_stats[grade_idx].max_allowed > 0 &&
      _grade_stats[grade_idx].loaded_count >= _grade_stats[grade_idx].max_allowed) {
    log_violation(grade, class_name, "grade limit reached");
    return false;
  }

  // Check grade threshold (lower grades face restriction after main is established)
  if (_grade_stats[CLASS_GRADE_MAIN].loaded_count > 0) {
    int main_grade = CLASS_GRADE_MAIN;
    int min_allowed_grade = main_grade - _grade_threshold;
    if ((int)grade < min_allowed_grade && grade != CLASS_GRADE_UNGRADED) {
      // This grade is below the threshold — apply stricter check
      // Allow only 50% of the configured max for sub-threshold grades
      int effective_max = _grade_stats[grade_idx].max_allowed / 2;
      if (effective_max > 0 && _grade_stats[grade_idx].loaded_count >= effective_max) {
        log_violation(grade, class_name, "sub-threshold grade limit (50% cap)");
        return false;
      }
    }
  }

  return true;
}

// ============================================================================
// Violation Logging
// ============================================================================

void ClassLoadGuard::log_violation(ClassGrade grade, const char* class_name, const char* reason) {
  int grade_idx = (int)grade;
  _grade_stats[grade_idx].warned_count++;

  log_warning(class, load)("ClassLoadGuard: %s [class=%s, grade=%s(%d), loaded=%d, max=%d]",
                           reason, class_name, grade_name(grade), (int)grade,
                           _grade_stats[grade_idx].loaded_count,
                           _grade_stats[grade_idx].max_allowed);
}

// ============================================================================
// Main Gate
// ============================================================================

bool ClassLoadGuard::allow_class_load(const char* class_name,
                                      const unsigned char* bytecode, size_t bytecode_len,
                                      const char* loader_name) {
  if (!_enabled) return true;

  // Always allow JDK internal classes (java.*, jdk.*, sun.*)
  if (class_name != nullptr) {
    if (strncmp(class_name, "java/", 5) == 0 ||
        strncmp(class_name, "jdk/", 4) == 0 ||
        strncmp(class_name, "sun/", 4) == 0 ||
        strncmp(class_name, "javax/", 6) == 0 ||
        strncmp(class_name, "com/sun/", 8) == 0 ||
        strncmp(class_name, "org/xml/", 8) == 0 ||
        strncmp(class_name, "org/w3c/", 8) == 0) {
      return true;
    }
  }

  // Detect grade
  ClassGrade grade = detect_grade(class_name, bytecode, bytecode_len);

  // Lock for stats update
  lock();

  // Check quantity limits
  bool allowed = check_quantity(grade, class_name);

  if (allowed) {
    // Record the load
    _global_loaded_count++;
    _grade_stats[(int)grade].loaded_count++;
  } else {
    _grade_stats[(int)grade].refused_count++;

    // Apply policy
    switch (_policy) {
      case GUARD_POLICY_WARN:
        // Already logged in check_quantity, allow anyway
        _global_loaded_count++;
        _grade_stats[(int)grade].loaded_count++;
        allowed = true;
        break;

      case GUARD_POLICY_SOFT:
        // Log, delay, then allow
        unlock();
        os::naked_short_sleep(10);  // 10ms penalty
        lock();
        _global_loaded_count++;
        _grade_stats[(int)grade].loaded_count++;
        allowed = true;
        break;

      case GUARD_POLICY_HARD:
        // Refuse — allowed stays false
        break;
    }
  }

  unlock();
  return allowed;
}

// ============================================================================
// Main Class Registration
// ============================================================================

void ClassLoadGuard::register_main_class(const char* class_name) {
  if (!_enabled) return;

  lock();
  _grade_stats[CLASS_GRADE_MAIN].loaded_count++;
  _global_loaded_count++;
  unlock();

  log_info(class, load)("ClassLoadGuard: main class registered: %s", class_name);
}

// ============================================================================
// Diagnostics
// ============================================================================

void ClassLoadGuard::print_stats(outputStream* st) {
  st->print_cr("ClassLoadGuard Statistics:");
  st->print_cr("  Enabled: %s", _enabled ? "yes" : "no");
  st->print_cr("  Policy: %s", _policy == GUARD_POLICY_HARD ? "HARD" :
                                _policy == GUARD_POLICY_SOFT ? "SOFT" : "WARN");
  st->print_cr("  Global: %d loaded / %d max", _global_loaded_count, _global_max_classes);
  st->print_cr("  Grade threshold: %d (grades below %d face restrictions)",
               _grade_threshold, CLASS_GRADE_MAIN - _grade_threshold);
  st->cr();
  st->print_cr("  Grade          | Loaded | Max    | Refused | Warned");
  st->print_cr("  ---------------+--------+--------+---------+-------");
  for (int i = CLASS_GRADE_COUNT - 1; i >= 0; i--) {
    st->print_cr("  %-14s | %6d | %6d | %7d | %6d",
                 grade_name((ClassGrade)i),
                 _grade_stats[i].loaded_count,
                 _grade_stats[i].max_allowed,
                 _grade_stats[i].refused_count,
                 _grade_stats[i].warned_count);
  }
}

void ClassLoadGuard::print_config(outputStream* st) {
  st->print_cr("ClassLoadGuard Configuration:");
  st->print_cr("  -XX:+ClassLoadGuard                    (enable)");
  st->print_cr("  -XX:ClassLoadGuardPolicy=warn|soft|hard");
  st->print_cr("  -XX:ClassLoadGuardGlobalMax=N");
  st->print_cr("  -XX:ClassLoadGuardThreshold=N          (grade distance from main)");
  st->print_cr("  -XX:ClassLoadGuardGradeMax=GRADE:N     (per-grade ceiling)");
}
