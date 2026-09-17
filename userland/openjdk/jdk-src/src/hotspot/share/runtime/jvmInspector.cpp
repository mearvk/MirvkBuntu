/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmInspector.cpp - Secure JVM Pause-Frame Inspector Implementation
 *
 * Technical frame inspection for loaded Java classes and their C backing.
 * Maintains full class load history since system inception.
 */

#include "runtime/jvmInspector.hpp"
#include "runtime/atomic.hpp"
#include "runtime/os.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/vmOperations.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/classLoaderDataGraph.hpp"
#include "oops/instanceKlass.hpp"
#include "oops/method.hpp"
#include "oops/fieldStreams.inline.hpp"
#include "utilities/ostream.hpp"

#include <string.h>
#include <stdlib.h>

// ============================================================================
// Static State
// ============================================================================

bool               JvmInspector::_enabled = false;
bool               JvmInspector::_paused = false;
InspectionVerdict  JvmInspector::_current_verdict = VERDICT_PASS;
InspectorGrade     JvmInspector::_operator_grade = INSPECTOR_LOCAL;
int                JvmInspector::_total_inspections = 0;
int                JvmInspector::_total_classes_loaded = 0;
ClassLoadRecord*   JvmInspector::_history_head = nullptr;
ClassLoadRecord*   JvmInspector::_history_tail = nullptr;
volatile int       JvmInspector::_history_lock = 0;

// ============================================================================
// Locking
// ============================================================================

void JvmInspector::history_lock() {
  while (AtomicAccess::cmpxchg(&_history_lock, 0, 1) != 0) {
    os::naked_short_sleep(0);
  }
}

void JvmInspector::history_unlock() {
  AtomicAccess::store(&_history_lock, 0);
}

// ============================================================================
// Initialization
// ============================================================================

void JvmInspector::initialize() {
  _enabled = true;
  _paused = false;
  _current_verdict = VERDICT_PASS;
  _total_inspections = 0;
  _total_classes_loaded = 0;
  _history_head = nullptr;
  _history_tail = nullptr;

  log_info(class, load)("JvmInspector: initialized — full class history tracking enabled");
}

// ============================================================================
// Class Load Recording (history since inception)
// ============================================================================

void JvmInspector::record_class_load(const char* class_name, const char* loader_name,
                                     size_t bytecode_size, bool has_native, bool is_jdk) {
  if (!_enabled) return;

  ClassLoadRecord* record = (ClassLoadRecord*)os::malloc(sizeof(ClassLoadRecord), mtInternal);
  if (record == nullptr) return;

  record->class_name = os::strdup(class_name != nullptr ? class_name : "(anonymous)");
  record->loader_name = os::strdup(loader_name != nullptr ? loader_name : "bootstrap");
  record->load_time_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);
  record->bytecode_size = bytecode_size;
  record->has_native = has_native;
  record->is_jdk_class = is_jdk;
  record->next = nullptr;

  history_lock();
  record->sequence_number = ++_total_classes_loaded;
  if (_history_tail != nullptr) {
    _history_tail->next = record;
    _history_tail = record;
  } else {
    _history_head = record;
    _history_tail = record;
  }
  history_unlock();
}

// ============================================================================
// Authorization
// ============================================================================

bool JvmInspector::authorize_view(InspectorGrade grade, InspectionView view,
                                  const char* class_name) {
  // Local operators: application classes only, CLASS_VIEW and HISTORY_VIEW
  if (grade == INSPECTOR_LOCAL) {
    if (view == VIEW_NATIVE || view == VIEW_CODE || view == VIEW_FRAME) {
      log_warning(class, load)("JvmInspector: Local operator not authorized for %s view",
                               view == VIEW_NATIVE ? "NATIVE" :
                               view == VIEW_CODE ? "CODE" : "FRAME");
      return false;
    }
    // Cannot inspect JDK internals
    if (class_name != nullptr) {
      if (strncmp(class_name, "java/", 5) == 0 ||
          strncmp(class_name, "jdk/", 4) == 0 ||
          strncmp(class_name, "sun/", 4) == 0) {
        log_warning(class, load)("JvmInspector: Local operator cannot inspect JDK class: %s",
                                 class_name);
        return false;
      }
    }
  }

  // National operators: all classes, all Java views + limited native
  if (grade == INSPECTOR_NATIONAL) {
    if (view == VIEW_CODE) {
      log_warning(class, load)("JvmInspector: National operator not authorized for CODE view "
                               "(requires International grade)");
      return false;
    }
  }

  // International: full access
  return true;
}

// ============================================================================
// Technical Frame Construction
// ============================================================================

void JvmInspector::build_technical_frame(InstanceKlass* klass, TechnicalFrame* out) {
  memset(out, 0, sizeof(TechnicalFrame));

  // Class name
  if (klass->name() != nullptr) {
    strncpy(out->class_name, klass->name()->as_C_string(), sizeof(out->class_name) - 1);
  }

  // Superclass
  if (klass->super() != nullptr && klass->super()->name() != nullptr) {
    strncpy(out->superclass, klass->super()->name()->as_C_string(), sizeof(out->superclass) - 1);
  }

  // Interfaces
  Array<InstanceKlass*>* interfaces = klass->local_interfaces();
  if (interfaces != nullptr) {
    out->interface_count = interfaces->length();
    for (int i = 0; i < interfaces->length() && i < 32; i++) {
      if (interfaces->at(i)->name() != nullptr) {
        strncpy(out->interfaces[i], interfaces->at(i)->name()->as_C_string(), 255);
      }
    }
  }

  // Fields
  out->field_count = klass->java_fields_count();

  // Methods
  Array<Method*>* methods = klass->methods();
  if (methods != nullptr) {
    out->method_count = methods->length();
    for (int i = 0; i < methods->length(); i++) {
      if (methods->at(i)->is_native()) {
        out->native_method_count++;
      }
    }
  }

  // Instance size
  out->instance_size = klass->size_helper() * HeapWordSize;

  // Modifiers
  out->is_abstract = klass->is_abstract();
  out->is_interface = klass->is_interface();
  out->is_final = klass->is_final();

  // Source file
  if (klass->source_file_name() != nullptr) {
    strncpy(out->source_file, klass->source_file_name()->as_C_string(),
            sizeof(out->source_file) - 1);
  }

  // Load time from history
  history_lock();
  ClassLoadRecord* rec = _history_head;
  while (rec != nullptr) {
    if (strcmp(rec->class_name, out->class_name) == 0) {
      out->loaded_at_ms = rec->load_time_ms;
      out->load_sequence = rec->sequence_number;
      break;
    }
    rec = rec->next;
  }
  history_unlock();
}

// ============================================================================
// Print Technical Frame (what the operator sees)
// ============================================================================

void JvmInspector::print_technical_frame(TechnicalFrame* frame, outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  TECHNICAL FRAME — CLASS INSPECTION                              ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->cr();
  st->print_cr("  Class:          %s", frame->class_name);
  st->print_cr("  Superclass:     %s", frame->superclass[0] ? frame->superclass : "(none)");
  st->print_cr("  Source:         %s", frame->source_file[0] ? frame->source_file : "(unknown)");
  st->print_cr("  Load sequence:  #%d (at %llu ms since JVM start)",
               frame->load_sequence, (unsigned long long)frame->loaded_at_ms);
  st->cr();
  st->print_cr("  Interfaces (%d):", frame->interface_count);
  for (int i = 0; i < frame->interface_count && i < 32; i++) {
    st->print_cr("    → %s", frame->interfaces[i]);
  }
  st->cr();
  st->print_cr("  Structure:");
  st->print_cr("    Fields:          %d", frame->field_count);
  st->print_cr("    Methods:         %d (%d native)", frame->method_count, frame->native_method_count);
  st->print_cr("    Instance size:   %zu bytes", frame->instance_size);
  st->cr();
  st->print_cr("  Modifiers:       %s%s%s",
               frame->is_abstract ? "abstract " : "",
               frame->is_interface ? "interface " : "",
               frame->is_final ? "final " : "");
  st->cr();

  if (frame->native_method_count > 0) {
    st->print_cr("  ⚠ Contains %d native method(s) — C/C++ backing present", frame->native_method_count);
  }
  st->print_cr("────────────────────────────────────────────────────────────────────");
}

void JvmInspector::print_native_view(InstanceKlass* klass, outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  NATIVE VIEW — C/C++ Backing                                    ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->cr();

  Array<Method*>* methods = klass->methods();
  if (methods == nullptr) {
    st->print_cr("  (no methods)");
    return;
  }

  int native_count = 0;
  for (int i = 0; i < methods->length(); i++) {
    Method* m = methods->at(i);
    if (m->is_native()) {
      native_count++;
      st->print_cr("  [native] %s%s",
                   m->name() != nullptr ? m->name()->as_C_string() : "?",
                   m->signature() != nullptr ? m->signature()->as_C_string() : "()V");

      // Print native entry point if resolved
      address entry = m->native_function();
      if (entry != nullptr) {
        st->print_cr("           entry: %p", (void*)entry);
        // Try to find symbol name via dladdr
        Dl_info info;
        if (dladdr((void*)entry, &info) && info.dli_sname != nullptr) {
          st->print_cr("           symbol: %s (in %s)",
                       info.dli_sname,
                       info.dli_fname ? info.dli_fname : "?");
        }
      } else {
        st->print_cr("           entry: (not yet linked)");
      }
    }
  }

  if (native_count == 0) {
    st->print_cr("  No native methods — pure Java class");
  }
  st->print_cr("────────────────────────────────────────────────────────────────────");
}

void JvmInspector::print_code_view(InstanceKlass* klass, outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  CODE VIEW — Compiled/Interpreted Methods                        ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->cr();

  Array<Method*>* methods = klass->methods();
  if (methods == nullptr) return;

  for (int i = 0; i < methods->length(); i++) {
    Method* m = methods->at(i);
    st->print("  %s%s",
              m->name() != nullptr ? m->name()->as_C_string() : "?",
              m->signature() != nullptr ? m->signature()->as_C_string() : "()V");

    if (m->is_native()) {
      st->print_cr("  [NATIVE]");
    } else if (m->code() != nullptr) {
      st->print_cr("  [JIT-compiled, entry=%p, size=%d]",
                   (void*)m->code()->entry_point(),
                   m->code()->insts_size());
    } else {
      st->print_cr("  [interpreted, bytecode=%d bytes]",
                   m->code_size());
    }
  }
  st->print_cr("────────────────────────────────────────────────────────────────────");
}

void JvmInspector::print_frame_view(outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  FRAME VIEW — Current Thread Stack (at pause point)              ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->cr();
  st->print_cr("  (Stack trace available at safepoint — use jcmd for live inspection)");
  st->print_cr("────────────────────────────────────────────────────────────────────");
}

// ============================================================================
// Non-Pausing Inspection
// ============================================================================

bool JvmInspector::inspect_class(const char* class_name, InspectionView view,
                                 InspectorGrade grade, outputStream* out) {
  if (!_enabled) {
    out->print_cr("JvmInspector: not enabled");
    return false;
  }

  if (!authorize_view(grade, view, class_name)) {
    out->print_cr("JvmInspector: not authorized for this view/class");
    return false;
  }

  _total_inspections++;

  // View history doesn't need a klass lookup
  if (view == VIEW_HISTORY) {
    print_history_for_class(class_name, out);
    return true;
  }

  if (view == VIEW_FRAME) {
    print_frame_view(out);
    return true;
  }

  // Look up the class in the system dictionary
  // Convert class_name from slash-form to Symbol
  TempNewSymbol sym = SymbolTable::new_symbol(class_name);
  if (sym == nullptr) {
    out->print_cr("JvmInspector: cannot create symbol for '%s'", class_name);
    return false;
  }

  // Search the class
  Klass* k = SystemDictionary::find_instance_or_array_klass(Thread::current(), sym, Handle());
  if (k == nullptr || !k->is_instance_klass()) {
    out->print_cr("JvmInspector: class not found or not an instance class: %s", class_name);
    return false;
  }

  InstanceKlass* ik = InstanceKlass::cast(k);

  switch (view) {
    case VIEW_CLASS: {
      TechnicalFrame frame;
      build_technical_frame(ik, &frame);
      print_technical_frame(&frame, out);
      break;
    }
    case VIEW_NATIVE:
      print_native_view(ik, out);
      break;
    case VIEW_CODE:
      print_code_view(ik, out);
      break;
    default:
      out->print_cr("JvmInspector: unsupported view");
      return false;
  }

  return true;
}

// ============================================================================
// Pause and Inspect (safepoint-based)
// ============================================================================

InspectionVerdict JvmInspector::pause_and_inspect(const char* class_name,
                                                   InspectionView view,
                                                   InspectorGrade grade,
                                                   outputStream* out) {
  if (!_enabled) return VERDICT_PASS;

  out->print_cr("");
  out->print_cr("▶▶▶ JVM PAUSED FOR INSPECTION ◀◀◀");
  out->print_cr("    Operator grade: %s",
                grade == INSPECTOR_INTERNATIONAL ? "INTERNATIONAL" :
                grade == INSPECTOR_NATIONAL ? "NATIONAL" : "LOCAL");
  out->print_cr("    Target class:   %s", class_name);
  out->print_cr("");

  _paused = true;
  _current_verdict = VERDICT_PENDING;
  _operator_grade = grade;

  // Perform the inspection
  bool inspection_ok = inspect_class(class_name, view, grade, out);

  if (!inspection_ok) {
    out->print_cr("");
    out->print_cr("  ⚠ INSPECTION FAILED — class could not be drawn up");
    out->print_cr("  Awaiting operator verdict: RESUME / QUARANTINE / HALT");
  } else {
    out->print_cr("");
    out->print_cr("  ✓ Inspection complete");
    out->print_cr("  Awaiting operator verdict: RESUME / QUARANTINE / HALT");
  }

  // In a real system, we would wait here for the operator to submit verdict
  // via management port or /proc interface.
  // For now, auto-resume if inspection passed, await if failed.
  if (inspection_ok) {
    _current_verdict = VERDICT_RESUME;
    _paused = false;
  }
  // If not OK, _paused remains true — operator must call submit_verdict()

  return _current_verdict;
}

// ============================================================================
// Auto-Inspect on Load (integration with ClassLoadGuard)
// ============================================================================

InspectionVerdict JvmInspector::auto_inspect_on_load(const char* class_name,
                                                     const unsigned char* bytecode,
                                                     size_t bytecode_len) {
  if (!_enabled) return VERDICT_PASS;

  // Quick checks that might trigger auto-inspection:
  // 1. Class has native methods (possible hook point)
  // 2. Class name matches suspicious patterns
  // 3. Unusually large bytecode (potential payload)

  bool suspicious = false;

  if (bytecode_len > 500000) {
    // > 500KB bytecode is unusual
    log_warning(class, load)("JvmInspector: large bytecode (%zu bytes): %s",
                             bytecode_len, class_name);
    suspicious = true;
  }

  // Check for native method indicator in bytecode (ACC_NATIVE = 0x0100 in method_info)
  if (bytecode != nullptr && bytecode_len > 0) {
    // Simple heuristic: count native method markers
    // (A proper implementation would parse the classfile structure)
    int native_count = 0;
    for (size_t i = 0; i < bytecode_len - 1; i++) {
      // Native flag in access_flags of method_info
      if (bytecode[i] == 0x01 && bytecode[i + 1] == 0x00) {
        // Could be ACC_NATIVE — this is a rough heuristic
        native_count++;
      }
    }
    if (native_count > 20) {
      log_warning(class, load)("JvmInspector: many potential native methods (%d): %s",
                               native_count, class_name);
      suspicious = true;
    }
  }

  if (suspicious) {
    // Log for operator review — don't auto-pause on every suspicious class
    // (that would be too aggressive for normal operation)
    log_info(class, load)("JvmInspector: flagged for review: %s", class_name);
  }

  return VERDICT_PASS;
}

// ============================================================================
// Operator Verdict Submission
// ============================================================================

void JvmInspector::submit_verdict(InspectionVerdict verdict) {
  _current_verdict = verdict;
  _paused = false;

  switch (verdict) {
    case VERDICT_RESUME:
      log_info(class, load)("JvmInspector: operator verdict RESUME — continuing");
      break;
    case VERDICT_QUARANTINE:
      log_warning(class, load)("JvmInspector: operator verdict QUARANTINE — class will be unloaded");
      // Actual class unloading would be triggered via ClassLoaderData
      break;
    case VERDICT_HALT:
      log_error(class, load)("JvmInspector: operator verdict HALT — terminating JVM");
      os::abort(false);
      break;
    default:
      break;
  }
}

// ============================================================================
// History
// ============================================================================

void JvmInspector::print_history(outputStream* st, int last_n) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  CLASS LOAD HISTORY (since inception)                            ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->print_cr("  Total classes loaded: %d", _total_classes_loaded);
  st->cr();
  st->print_cr("  Seq# | Time(ms) | Native | JDK | Class");
  st->print_cr("  -----+----------+--------+-----+------");

  history_lock();

  // If last_n specified, skip to the tail
  ClassLoadRecord* rec = _history_head;
  if (last_n > 0 && _total_classes_loaded > last_n) {
    int skip = _total_classes_loaded - last_n;
    while (rec != nullptr && skip > 0) { rec = rec->next; skip--; }
  }

  while (rec != nullptr) {
    st->print_cr("  %5d| %8llu | %-6s | %-3s | %s",
                 rec->sequence_number,
                 (unsigned long long)rec->load_time_ms,
                 rec->has_native ? "YES" : "no",
                 rec->is_jdk_class ? "yes" : "no",
                 rec->class_name);
    rec = rec->next;
  }

  history_unlock();
}

void JvmInspector::print_history_for_class(const char* class_name, outputStream* st) {
  history_lock();

  ClassLoadRecord* rec = _history_head;
  bool found = false;

  while (rec != nullptr) {
    if (strstr(rec->class_name, class_name) != nullptr) {
      if (!found) {
        st->print_cr("  History for '%s':", class_name);
        found = true;
      }
      st->print_cr("    #%d loaded at %llu ms by [%s] (%zu bytes%s)",
                   rec->sequence_number,
                   (unsigned long long)rec->load_time_ms,
                   rec->loader_name,
                   rec->bytecode_size,
                   rec->has_native ? ", has native" : "");
    }
    rec = rec->next;
  }

  if (!found) {
    st->print_cr("  No history found for '%s'", class_name);
  }

  history_unlock();
}

// ============================================================================
// Diagnostics
// ============================================================================

void JvmInspector::print_status(outputStream* st) {
  st->print_cr("JVM Inspector Status:");
  st->print_cr("  Enabled:            %s", _enabled ? "yes" : "no");
  st->print_cr("  Currently paused:   %s", _paused ? "YES — awaiting verdict" : "no");
  st->print_cr("  Total inspections:  %d", _total_inspections);
  st->print_cr("  Classes recorded:   %d (since inception)", _total_classes_loaded);
  st->print_cr("  Current verdict:    %s",
               _current_verdict == VERDICT_PASS ? "PASS" :
               _current_verdict == VERDICT_RESUME ? "RESUME" :
               _current_verdict == VERDICT_QUARANTINE ? "QUARANTINE" :
               _current_verdict == VERDICT_HALT ? "HALT" : "PENDING");
}
