/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * classLoadGuard.hpp - Class Loading Safety Guardian
 *
 * Provides quantity and quality controls for class loading:
 *
 * QUANTITY CONTROLS:
 *   - Global class count ceiling (max classes loadable by the JVM)
 *   - Per-loader class count limits
 *   - Per-grade quotas (e.g., max 50 Substitute classes total)
 *
 * QUALITY CONTROLS (Class Grades):
 *   Classes are graded by their architectural role. Each grade has a
 *   trust level relative to the main application class. Higher grades
 *   are more trusted; lower grades are constrained.
 *
 *   Grade | Name        | Role                                      | Trust
 *   ------+-------------+-------------------------------------------+------
 *     7   | Main        | Application entry point (main class)      | Full
 *     6   | Manager     | Orchestrates other classes, lifecycle mgmt | High
 *     5   | Archetype   | Abstract base / template pattern          | High
 *     4   | Builder     | Constructs complex objects (Builder/Factory)| Medium
 *     3   | Inheritor   | Extends archetypes, concrete impl         | Medium
 *     2   | Gainer      | Accumulates state/data, caches            | Low
 *     1   | Substitute  | Proxy, adapter, decorator, wrapper        | Low
 *     0   | Ungraded    | Default for classes without annotation    | Baseline
 *
 * POLICY:
 *   - Main class always loads (grade 7)
 *   - Classes at grade <= (main_grade - threshold) face stricter limits
 *   - Each grade has configurable max count
 *   - Violations: WARN (log + continue), SOFT (delay + warn), HARD (refuse load)
 *
 * CONFIGURATION:
 *   Via jvm-config.xml <class-load-guard> section or -XX: flags.
 */

#ifndef SHARE_CLASSFILE_CLASSLOADGUARD_HPP
#define SHARE_CLASSFILE_CLASSLOADGUARD_HPP

#include "memory/allocation.hpp"
#include "runtime/os.hpp"
#include "utilities/ostream.hpp"

// Class architectural grades
enum ClassGrade {
  CLASS_GRADE_UNGRADED   = 0,  // No annotation — default baseline
  CLASS_GRADE_SUBSTITUTE = 1,  // Proxy, adapter, decorator, wrapper
  CLASS_GRADE_GAINER     = 2,  // Accumulates state, caches, aggregators
  CLASS_GRADE_INHERITOR  = 3,  // Concrete extension of an archetype
  CLASS_GRADE_BUILDER    = 4,  // Factory, builder pattern, constructors
  CLASS_GRADE_ARCHETYPE  = 5,  // Abstract base, template, interface contract
  CLASS_GRADE_MANAGER    = 6,  // Orchestrator, lifecycle, coordination
  CLASS_GRADE_MAIN       = 7,  // Application entry point

  CLASS_GRADE_COUNT      = 8
};

// Enforcement policy when a limit is exceeded
enum GuardPolicy {
  GUARD_POLICY_WARN = 0,   // Log warning, allow load
  GUARD_POLICY_SOFT = 1,   // Log warning, add artificial delay, allow
  GUARD_POLICY_HARD = 2    // Refuse to load (throws ClassNotFoundException)
};

// Per-grade statistics
struct GradeStats {
  int     loaded_count;       // How many of this grade are loaded
  int     max_allowed;        // Configured ceiling (-1 = unlimited)
  int     refused_count;      // How many were refused
  int     warned_count;       // How many triggered warnings
};

class ClassLoadGuard : public AllStatic {
private:
  // Global state
  static bool         _enabled;
  static int          _global_max_classes;      // Total class ceiling (-1 = unlimited)
  static int          _global_loaded_count;     // Total classes loaded
  static int          _grade_threshold;         // Grades below (main - threshold) are restricted
  static GuardPolicy  _policy;                  // Enforcement level
  static GradeStats   _grade_stats[CLASS_GRADE_COUNT];

  // Lock for thread-safe updates
  static volatile int _lock;

  // Internal
  static void lock();
  static void unlock();
  static ClassGrade detect_grade(const char* class_name, const unsigned char* bytecode, size_t len);
  static ClassGrade grade_from_annotation(const unsigned char* bytecode, size_t len);
  static ClassGrade grade_from_name_heuristic(const char* class_name);
  static bool check_quantity(ClassGrade grade, const char* class_name);
  static void log_violation(ClassGrade grade, const char* class_name, const char* reason);

public:
  // Initialize from JVM flags or XML config
  static void initialize();

  // Main gate — called before every class load completes
  // Returns true if class is allowed to load, false if refused
  static bool allow_class_load(const char* class_name,
                               const unsigned char* bytecode, size_t bytecode_len,
                               const char* loader_name);

  // Register the main class (sets grade 7 baseline)
  static void register_main_class(const char* class_name);

  // Configuration
  static void set_enabled(bool enabled)         { _enabled = enabled; }
  static void set_global_max(int max)           { _global_max_classes = max; }
  static void set_grade_max(ClassGrade g, int max) { _grade_stats[(int)g].max_allowed = max; }
  static void set_policy(GuardPolicy p)         { _policy = p; }
  static void set_grade_threshold(int t)        { _grade_threshold = t; }

  // Diagnostics
  static void print_stats(outputStream* st);
  static void print_config(outputStream* st);
  static int  total_loaded()                    { return _global_loaded_count; }
  static int  grade_loaded(ClassGrade g)        { return _grade_stats[(int)g].loaded_count; }

  // Grade name lookup
  static const char* grade_name(ClassGrade g);
};

#endif // SHARE_CLASSFILE_CLASSLOADGUARD_HPP
