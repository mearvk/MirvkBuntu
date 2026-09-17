/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmCodex.cpp - Static System Codex Implementation
 *
 * In-resident module registry. Codex entries sit in-in and inform
 * neighboring code about self, altitude, relevance, timing, and
 * signal destiny. Normed to NC English Speaking US standard.
 */

#include "runtime/jvmCodex.hpp"
#include "runtime/os.hpp"
#include "runtime/atomic.hpp"
#include "utilities/ostream.hpp"

#include <string.h>
#include <stdlib.h>

// ============================================================================
// Static State
// ============================================================================

bool         JvmCodex::_enabled = false;
CodexEntry*  JvmCodex::_registry_head = nullptr;
int          JvmCodex::_next_codex_id = 1;
int          JvmCodex::_total_entries = 0;
volatile int JvmCodex::_lock = 0;

// ============================================================================
// Locking
// ============================================================================

void JvmCodex::lock() {
  while (AtomicAccess::cmpxchg(&_lock, 0, 1) != 0) {
    os::naked_short_sleep(0);
  }
}

void JvmCodex::unlock() {
  AtomicAccess::store(&_lock, 0);
}

// ============================================================================
// Name Lookups
// ============================================================================

const char* JvmCodex::shape_name(CodexShape s) {
  switch (s) {
    case SHAPE_MODULE:    return "Module";
    case SHAPE_CLASS:     return "Class";
    case SHAPE_FUNCTION:  return "Function";
    case SHAPE_DATA:      return "Data";
    case SHAPE_INTERFACE: return "Interface";
    case SHAPE_SYSTEM:    return "System Center";
    default:              return "Unknown";
  }
}

const char* JvmCodex::color_name(CodexColor c) {
  switch (c) {
    case COLOR_WHITE:  return "White (Ethics/Safety)";
    case COLOR_BLUE:   return "Blue (Communication)";
    case COLOR_GREEN:  return "Green (Growth/Data)";
    case COLOR_GOLD:   return "Gold (Authority)";
    case COLOR_RED:    return "Red (Security/Critical)";
    case COLOR_SILVER: return "Silver (Utility)";
    case COLOR_CLEAR:  return "Clear (Pure Logic)";
    default:           return "Unknown";
  }
}

const char* JvmCodex::rigor_name(RigorLevel r) {
  switch (r) {
    case RIGOR_DRAFT:     return "Draft";
    case RIGOR_REVIEWED:  return "Reviewed";
    case RIGOR_TESTED:    return "Tested";
    case RIGOR_CERTIFIED: return "Certified";
    case RIGOR_CANONICAL: return "Canonical";
    default:              return "Unknown";
  }
}

const char* JvmCodex::grade_name(InstallerGrade g) {
  switch (g) {
    case GRADE_USER_III:      return "User III";
    case GRADE_TECH_II_PLUS:  return "Tech II+";
    case GRADE_INSTALLER_IV:  return "Installer IV+";
    case GRADE_NORMAL_VI_PP:  return "Normal VI++";
    default:                  return "Unknown";
  }
}

// ============================================================================
// Permission Checks
// ============================================================================

bool JvmCodex::check_install_permission(InstallerGrade grade) {
  // Installer IV+ or Normal VI++ can install
  return grade >= GRADE_INSTALLER_IV;
}

bool JvmCodex::check_read_permission(InstallerGrade grade, CodexEntry* entry) {
  if (entry == nullptr) return false;

  // User III: can read name, size, shape, color
  // Tech II+: can read code/functionality
  // All grades can see that it exists (it is in-in)
  (void)grade;  // All reads are permitted at minimum level
  return true;
}

// ============================================================================
// Initialization
// ============================================================================

void JvmCodex::initialize() {
  _enabled = true;
  log_info(os)("JvmCodex: initialized — in-resident module registry (NC US standard)");
}

// ============================================================================
// INSTALL — Place a codex entry in-in
// ============================================================================

int JvmCodex::install_codex(const char* name, CodexShape shape, CodexColor color,
                            const char* functionality, RigorLevel rigor,
                            const char* improvement,
                            const char* code, size_t code_len,
                            InstallerGrade installer_grade,
                            const char* installer_name) {
  if (!_enabled) return -1;

  // Permission check
  if (!check_install_permission(installer_grade)) {
    log_warning(os)("JvmCodex: PERMISSION DENIED — grade %s cannot install (need Installer IV+)",
                    grade_name(installer_grade));
    return -1;
  }

  CodexEntry* entry = (CodexEntry*)os::malloc(sizeof(CodexEntry), mtInternal);
  if (entry == nullptr) return -1;

  memset(entry, 0, sizeof(CodexEntry));

  lock();
  entry->codex_id = _next_codex_id++;

  strncpy(entry->name, name ? name : "unnamed", sizeof(entry->name) - 1);
  entry->size = code_len;
  entry->shape = shape;
  entry->color = color;
  strncpy(entry->functionality, functionality ? functionality : "",
          sizeof(entry->functionality) - 1);
  entry->rigor = rigor;
  strncpy(entry->improvement, improvement ? improvement : "none",
          sizeof(entry->improvement) - 1);
  entry->installed_by = installer_grade;
  strncpy(entry->installer_name, installer_name ? installer_name : "system",
          sizeof(entry->installer_name) - 1);
  entry->installed_at_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);
  entry->active = true;

  // Copy code content if provided
  if (code != nullptr && code_len > 0) {
    char* code_copy = (char*)os::malloc(code_len + 1, mtInternal);
    if (code_copy != nullptr) {
      memcpy(code_copy, code, code_len);
      code_copy[code_len] = '\0';
      entry->code = code_copy;
      entry->code_len = code_len;
    }
  } else {
    entry->code = nullptr;
    entry->code_len = 0;
  }

  entry->next = _registry_head;
  _registry_head = entry;
  _total_entries++;
  unlock();

  log_info(os)("JvmCodex: INSTALLED [id=%d, name=%s, shape=%s, color=%s, rigor=%s, by=%s (%s)]",
               entry->codex_id, entry->name, shape_name(shape), color_name(color),
               rigor_name(rigor), installer_name, grade_name(installer_grade));

  return entry->codex_id;
}

// ============================================================================
// REFERENCE — Lookup
// ============================================================================

CodexEntry* JvmCodex::find_by_name(const char* name) {
  if (name == nullptr) return nullptr;

  lock();
  CodexEntry* e = _registry_head;
  while (e != nullptr) {
    if (strcmp(e->name, name) == 0 && e->active) {
      unlock();
      return e;
    }
    e = e->next;
  }
  unlock();
  return nullptr;
}

CodexEntry* JvmCodex::find_by_id(int codex_id) {
  lock();
  CodexEntry* e = _registry_head;
  while (e != nullptr) {
    if (e->codex_id == codex_id) {
      unlock();
      return e;
    }
    e = e->next;
  }
  unlock();
  return nullptr;
}

// ============================================================================
// QUERY — Properties
// ============================================================================

const char* JvmCodex::get_name(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->name : nullptr;
}

size_t JvmCodex::get_size(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->size : 0;
}

CodexShape JvmCodex::get_shape(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->shape : SHAPE_DATA;
}

CodexColor JvmCodex::get_color(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->color : COLOR_CLEAR;
}

const char* JvmCodex::get_code(int codex_id, InstallerGrade requester_grade) {
  if (requester_grade < GRADE_TECH_II_PLUS) {
    return nullptr;  // User III cannot see code
  }
  CodexEntry* e = find_by_id(codex_id);
  return (e && e->active) ? e->code : nullptr;
}

const char* JvmCodex::get_functionality(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->functionality : nullptr;
}

RigorLevel JvmCodex::get_rigor(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->rigor : RIGOR_DRAFT;
}

const char* JvmCodex::get_improvement(int codex_id) {
  CodexEntry* e = find_by_id(codex_id);
  return e ? e->improvement : nullptr;
}

// ============================================================================
// WITHDRAW / REACTIVATE
// ============================================================================

bool JvmCodex::withdraw_codex(int codex_id, InstallerGrade grade) {
  if (!check_install_permission(grade)) return false;

  CodexEntry* e = find_by_id(codex_id);
  if (e == nullptr) return false;

  e->active = false;
  log_info(os)("JvmCodex: WITHDRAWN [id=%d, name=%s]", codex_id, e->name);
  return true;
}

bool JvmCodex::reactivate_codex(int codex_id, InstallerGrade grade) {
  if (!check_install_permission(grade)) return false;

  CodexEntry* e = find_by_id(codex_id);
  if (e == nullptr) return false;

  e->active = true;
  log_info(os)("JvmCodex: REACTIVATED [id=%d, name=%s]", codex_id, e->name);
  return true;
}

// ============================================================================
// INVENTORY
// ============================================================================

int JvmCodex::active_entries() {
  int count = 0;
  lock();
  CodexEntry* e = _registry_head;
  while (e != nullptr) {
    if (e->active) count++;
    e = e->next;
  }
  unlock();
  return count;
}

void JvmCodex::print_registry(outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  SYSTEM CODEX REGISTRY (In-Resident Modules)                     ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->print_cr("  Standard: English Speaking United States (North Carolina)");
  st->print_cr("  Total entries: %d (%d active)", _total_entries, active_entries());
  st->cr();
  st->print_cr("  ID | Status | Shape        | Color           | Rigor     | Name");
  st->print_cr("  ---+--------+--------------+-----------------+-----------+------");

  lock();
  CodexEntry* e = _registry_head;
  while (e != nullptr) {
    st->print_cr("  %2d | %-6s | %-12s | %-15s | %-9s | %s",
                 e->codex_id,
                 e->active ? "IN-IN" : "OUT",
                 shape_name(e->shape),
                 color_name(e->color),
                 rigor_name(e->rigor),
                 e->name);
    e = e->next;
  }
  unlock();
}

void JvmCodex::print_entry(int codex_id, outputStream* st) {
  CodexEntry* e = find_by_id(codex_id);
  if (e == nullptr) {
    st->print_cr("  Codex #%d not found.", codex_id);
    return;
  }

  st->print_cr("  ┌─────────────────────────────────────────────────────────");
  st->print_cr("  │ CODEX #%d: %s", e->codex_id, e->name);
  st->print_cr("  ├─────────────────────────────────────────────────────────");
  st->print_cr("  │ Status:        %s", e->active ? "IN-IN (active, resident)" : "WITHDRAWN");
  st->print_cr("  │ Shape:         %s", shape_name(e->shape));
  st->print_cr("  │ Color:         %s", color_name(e->color));
  st->print_cr("  │ Size:          %zu bytes", e->size);
  st->print_cr("  │ Rigor:         %s", rigor_name(e->rigor));
  st->print_cr("  │ Functionality: %s", e->functionality);
  st->print_cr("  │ Improvement:   %s", e->improvement);
  st->print_cr("  │ Installed by:  %s (%s)", e->installer_name, grade_name(e->installed_by));
  st->print_cr("  │ Installed at:  %llu ms since JVM start",
               (unsigned long long)e->installed_at_ms);
  if (e->code != nullptr) {
    st->print_cr("  │ Code:          present (%zu bytes)", e->code_len);
  } else {
    st->print_cr("  │ Code:          (reference only, no embedded content)");
  }
  st->print_cr("  └─────────────────────────────────────────────────────────");
}

// ============================================================================
// Diagnostics
// ============================================================================

void JvmCodex::print_status(outputStream* st) {
  st->print_cr("JVM Codex Status:");
  st->print_cr("  Enabled:          %s", _enabled ? "yes" : "no");
  st->print_cr("  Total entries:    %d", _total_entries);
  st->print_cr("  Active (in-in):   %d", active_entries());
  st->print_cr("  Standard:         NC English Speaking US");
  st->cr();
  st->print_cr("  Installer Grades:");
  st->print_cr("    User III       — read name/size/shape/color");
  st->print_cr("    Tech II+       — inspect code/functionality");
  st->print_cr("    Installer IV+  — install/withdraw codex entries");
  st->print_cr("    Normal VI++    — full normalized operation");
}
