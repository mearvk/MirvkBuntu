/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmInspector.hpp - Secure JVM Pause-Frame Inspector
 *
 * Allows an authorized operator (national or international grade) to:
 *   1. Pause the JVM at a safepoint
 *   2. Inspect any loaded class — its Java structure or underlying C frame
 *   3. Draw up a technical specification view of the class
 *   4. View any frame loaded since system inception
 *   5. Resume or halt if inspection fails
 *
 * OPERATOR GRADES:
 *   Grade 1 - Local       : Can inspect application classes only
 *   Grade 2 - National    : Can inspect all classes including JDK internals
 *   Grade 3 - International: Can inspect native (C/C++) frames and JIT-compiled code
 *
 * INSPECTION VIEWS:
 *   - CLASS_VIEW:  Java class structure (fields, methods, inheritance, interfaces)
 *   - NATIVE_VIEW: Underlying C/C++ backing (native methods, JNI entry points)
 *   - FRAME_VIEW:  Stack frame at time of pause (registers, locals, operands)
 *   - CODE_VIEW:   JIT-compiled machine code or interpreter bytecode
 *   - HISTORY_VIEW: Class load timeline since system inception
 *
 * PAUSE BEHAVIOR:
 *   - Pause is a safepoint — all Java threads halt cleanly
 *   - Inspection occurs while paused (no mutation possible)
 *   - If inspection reveals violation → operator can choose:
 *     a) RESUME — continue execution (pass)
 *     b) QUARANTINE — unload the offending class, continue
 *     c) HALT — terminate JVM with diagnostic dump
 *
 * INTERFACE:
 *   - Via /proc/jvm/inspector (kernel module integration)
 *   - Via jcmd <pid> JVM.inspect <class> <view> <operator_grade>
 *   - Via management port (secure socket, authenticated)
 */

#ifndef SHARE_RUNTIME_JVMINSPECTOR_HPP
#define SHARE_RUNTIME_JVMINSPECTOR_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"
#include "oops/instanceKlass.hpp"

// Operator authorization grades
enum InspectorGrade {
  INSPECTOR_LOCAL         = 1,  // Application classes only
  INSPECTOR_NATIONAL     = 2,  // All classes including JDK
  INSPECTOR_INTERNATIONAL = 3   // Native frames, JIT code, full system
};

// View types
enum InspectionView {
  VIEW_CLASS   = 0,   // Java class structure
  VIEW_NATIVE  = 1,   // C/C++ native backing
  VIEW_FRAME   = 2,   // Stack frame (registers, locals)
  VIEW_CODE    = 3,   // Compiled machine code or bytecode
  VIEW_HISTORY = 4    // Class load history since inception
};

// Inspection result / operator decision
enum InspectionVerdict {
  VERDICT_PASS       = 0,   // Inspection passed — resume
  VERDICT_RESUME     = 1,   // Operator says continue
  VERDICT_QUARANTINE = 2,   // Unload offending class, continue
  VERDICT_HALT       = 3,   // Terminate JVM
  VERDICT_PENDING    = 4    // Waiting for operator decision
};

// Record of a loaded class (kept since inception)
struct ClassLoadRecord {
  const char*   class_name;       // Fully qualified name (/ separated)
  const char*   loader_name;      // ClassLoader that loaded it
  uint64_t      load_time_ms;     // Time since JVM start (ms)
  int           sequence_number;  // Load order (1, 2, 3, ...)
  size_t        bytecode_size;    // Size of classfile in bytes
  bool          has_native;       // Contains native methods
  bool          is_jdk_class;     // From JDK modules
  ClassLoadRecord* next;          // Linked list
};

// Technical frame output (what the operator sees)
struct TechnicalFrame {
  char          class_name[512];
  char          superclass[256];
  int           interface_count;
  char          interfaces[32][256];
  int           field_count;
  int           method_count;
  int           native_method_count;
  size_t        instance_size;
  bool          is_abstract;
  bool          is_interface;
  bool          is_final;
  char          source_file[256];
  uint64_t      loaded_at_ms;
  int           load_sequence;
};

class JvmInspector : public AllStatic {
private:
  static bool               _enabled;
  static bool               _paused;
  static InspectionVerdict  _current_verdict;
  static InspectorGrade     _operator_grade;
  static int                _total_inspections;
  static int                _total_classes_loaded;

  // Class load history (linked list, never freed — history since inception)
  static ClassLoadRecord*   _history_head;
  static ClassLoadRecord*   _history_tail;
  static volatile int       _history_lock;

  // Internal
  static void history_lock();
  static void history_unlock();
  static bool authorize_view(InspectorGrade grade, InspectionView view, const char* class_name);
  static void build_technical_frame(InstanceKlass* klass, TechnicalFrame* out);
  static void print_technical_frame(TechnicalFrame* frame, outputStream* st);
  static void print_native_view(InstanceKlass* klass, outputStream* st);
  static void print_code_view(InstanceKlass* klass, outputStream* st);
  static void print_frame_view(outputStream* st);

public:
  // =========================================================================
  // Initialization
  // =========================================================================
  static void initialize();

  // =========================================================================
  // Class Load Recording (called on every class load — builds history)
  // =========================================================================
  static void record_class_load(const char* class_name, const char* loader_name,
                                size_t bytecode_size, bool has_native, bool is_jdk);

  // =========================================================================
  // PAUSE AND INSPECT
  // Main operator entry point. Pauses JVM and presents technical frame.
  // Returns the verdict (resume, quarantine, or halt).
  // =========================================================================
  static InspectionVerdict pause_and_inspect(const char* class_name,
                                             InspectionView view,
                                             InspectorGrade grade,
                                             outputStream* out);

  // =========================================================================
  // NON-PAUSING INSPECTION
  // Draw up a technical frame without pausing (read-only snapshot).
  // =========================================================================
  static bool inspect_class(const char* class_name, InspectionView view,
                            InspectorGrade grade, outputStream* out);

  // =========================================================================
  // HISTORY QUERY
  // Retrieve load records since inception.
  // =========================================================================
  static void print_history(outputStream* st, int last_n);
  static void print_history_for_class(const char* class_name, outputStream* st);
  static int  total_classes_recorded() { return _total_classes_loaded; }

  // =========================================================================
  // AUTOMATIC INSPECTION (called by ClassLoadGuard on suspicious loads)
  // If inspection fails, JVM is paused awaiting operator verdict.
  // =========================================================================
  static InspectionVerdict auto_inspect_on_load(const char* class_name,
                                                const unsigned char* bytecode,
                                                size_t bytecode_len);

  // =========================================================================
  // OPERATOR VERDICT (submitted by external operator via management channel)
  // =========================================================================
  static void submit_verdict(InspectionVerdict verdict);
  static bool is_paused() { return _paused; }
  static InspectionVerdict current_verdict() { return _current_verdict; }

  // =========================================================================
  // DIAGNOSTICS
  // =========================================================================
  static void print_status(outputStream* st);
};

#endif // SHARE_RUNTIME_JVMINSPECTOR_HPP
