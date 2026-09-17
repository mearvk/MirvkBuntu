/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmCodex.hpp - Static System Codex: In-Resident Module Registry
 *
 * A codex is a special, carefully installed piece of content that sits
 * in-in to the JVM — binary sees it as already present, static, and
 * careful. It is not loaded dynamically; it is registered at system
 * inception and remains safe, referenced by other code for:
 *
 *   - Size, shape, name, color
 *   - Code itself (functionality)
 *   - Improvement potential
 *   - Rigor of implementation
 *
 * Other program code sees the codex and continues operation knowing it
 * is sit carefully IN (not out). The codex informs neighboring modules
 * about self-awareness, altitude, relevance, operational timing, and
 * the capacity for signal destiny reacquisition.
 *
 * INSTALLER GRADES (normed, standard — English Speaking United States,
 * North Carolina):
 *
 *   User III       — End user, can reference codex for read/shape/name
 *   Tech II+       — Technical staff, can inspect code/functionality
 *   Installer IV+  — Can install and register new codex entries
 *   Normal VI++    — Normalized standard access, full operation
 *
 * CODEX PROPERTIES:
 *   Every registered codex has these attributes:
 *     - name         (identity)
 *     - size         (memory footprint)
 *     - shape        (structural form: module, class, function, data)
 *     - color        (operational domain/concern)
 *     - code         (the content itself)
 *     - functionality(what it does)
 *     - rigor        (implementation quality grade)
 *     - improvement  (known upgrade path or version)
 *
 * OPERATIONAL INTERFACE (ICodexAware):
 *   Modules that neighbor a codex implement this interface to express:
 *     - know_self()               — understand own identity relative to codex
 *     - know_altitude()           — hierarchical position in system
 *     - know_relevance()          — relationship to source/contriver
 *     - speed_of_base()           — base operational throughput
 *     - use_of_use()              — primary action invocation
 *     - reuse_of_contrived()      — secondary self-maintenance actions
 *     - when_to_peak()            — optimal execution timing
 *     - when_to_start()           — initialization signal
 *     - when_to_pause()           — quiescence signal
 *     - when_to_operate()         — active operation window
 *     - when_to_base_reoperate()  — restart after maintenance
 *     - when_to_startle()         — interrupt/alert handling
 *     - when_to_skimp()           — resource conservation mode
 *     - when_to_wonder()          — exploratory/heuristic mode
 *     - when_to_accept_novel()    — integration of new ideas
 *     - reacquire_signal_destiny()— restore purpose after disruption
 *
 * SAFETY:
 *   The codex and its interface are designed for careful future expansion.
 *   Users who extend these views will find strong signal destiny recovery
 *   built into the base. The system is normed to North Carolina English
 *   Speaking United States standard.
 */

#ifndef SHARE_RUNTIME_JVMCODEX_HPP
#define SHARE_RUNTIME_JVMCODEX_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"

// ============================================================================
// Installer Grades (NC Standard)
// ============================================================================

enum InstallerGrade {
  GRADE_USER_III      = 3,   // End user — read/shape/name access
  GRADE_TECH_II_PLUS  = 5,   // Technical staff — inspect code/functionality
  GRADE_INSTALLER_IV  = 7,   // Can install and register codex entries
  GRADE_NORMAL_VI_PP  = 9    // Normalized standard — full operation
};

// ============================================================================
// Codex Shape (structural form)
// ============================================================================

enum CodexShape {
  SHAPE_MODULE    = 0,  // Self-contained functional unit
  SHAPE_CLASS     = 1,  // Object-oriented structure
  SHAPE_FUNCTION  = 2,  // Single callable operation
  SHAPE_DATA      = 3,  // Static data/configuration
  SHAPE_INTERFACE = 4,  // Contract definition
  SHAPE_SYSTEM    = 5   // System-center or monger
};

// ============================================================================
// Codex Color (operational domain)
// ============================================================================

enum CodexColor {
  COLOR_WHITE   = 0,  // Ethics, integrity, safety
  COLOR_BLUE    = 1,  // Communication, networking, protocol
  COLOR_GREEN   = 2,  // Growth, data, accumulation
  COLOR_GOLD    = 3,  // Authority, management, orchestration
  COLOR_RED     = 4,  // Security, alerting, critical path
  COLOR_SILVER  = 5,  // Utility, tooling, infrastructure
  COLOR_CLEAR   = 6   // Pure logic, mathematics, computation
};

// ============================================================================
// Rigor Level
// ============================================================================

enum RigorLevel {
  RIGOR_DRAFT       = 0,  // Initial implementation
  RIGOR_REVIEWED    = 1,  // Peer reviewed
  RIGOR_TESTED      = 2,  // Unit + integration tested
  RIGOR_CERTIFIED   = 3,  // Formally verified / certified
  RIGOR_CANONICAL   = 4   // Reference implementation (gold standard)
};

// ============================================================================
// Codex Entry — a registered in-resident module
// ============================================================================

struct CodexEntry {
  int           codex_id;
  char          name[256];          // Identity
  size_t        size;               // Memory footprint (bytes)
  CodexShape    shape;              // Structural form
  CodexColor    color;              // Operational domain
  char          functionality[512]; // What it does (human description)
  RigorLevel    rigor;              // Implementation quality
  char          improvement[256];   // Known upgrade path
  InstallerGrade installed_by;      // Who put it in
  char          installer_name[128];// Installer identity
  uint64_t      installed_at_ms;    // When registered
  bool          active;             // Currently in-in (true) or withdrawn
  const char*   code;               // The content itself (or nullptr for reference)
  size_t        code_len;           // Length of code content
  CodexEntry*   next;
};

// ============================================================================
// ICodexAware — Interface for modules neighboring a codex
//
// Any code module that sits near a codex and wants to operate with
// awareness of self, timing, altitude, and signal destiny implements
// these functions. This is the base for future expansion.
// ============================================================================

class ICodexAware {
public:
  virtual ~ICodexAware() {}

  // === Self-Knowledge ===
  virtual const char* know_self() = 0;            // Own identity relative to codex
  virtual int         know_altitude() = 0;        // Hierarchical position (0=base, higher=above)
  virtual int         know_relevance() = 0;       // Relevance to source/contriver (0-100)
  virtual int         speed_of_base() = 0;        // Base operational throughput (ops/sec estimate)

  // === Use and Reuse ===
  virtual void        use_of_use() = 0;           // Primary action invocation
  virtual void        reuse_of_contrived() = 0;   // Secondary self-maintenance (attitude/respect)

  // === Timing Knowledge ===
  virtual bool        when_to_peak() = 0;         // Is now the time to execute at maximum?
  virtual bool        when_to_start() = 0;        // Should initialization begin?
  virtual bool        when_to_pause() = 0;        // Should operation quiesce?
  virtual bool        when_to_operate() = 0;      // Is the active operation window open?
  virtual bool        when_to_base_reoperate() = 0; // Should restart after maintenance?
  virtual bool        when_to_startle() = 0;      // Is interrupt/alert handling needed?
  virtual bool        when_to_skimp() = 0;        // Should conserve resources?
  virtual bool        when_to_wonder() = 0;       // Should explore heuristically?
  virtual bool        when_to_accept_novel() = 0; // Should integrate a new idea?

  // === Signal Destiny ===
  virtual bool        reacquire_signal_destiny() = 0; // Restore purpose after disruption
};

// ============================================================================
// JvmCodex — Static System Registry
// ============================================================================

class JvmCodex : public AllStatic {
private:
  static bool         _enabled;
  static CodexEntry*  _registry_head;
  static int          _next_codex_id;
  static int          _total_entries;
  static volatile int _lock;

  static void lock();
  static void unlock();

  // Permission check
  static bool check_install_permission(InstallerGrade grade);
  static bool check_read_permission(InstallerGrade grade, CodexEntry* entry);

public:
  // =========================================================================
  // Initialization
  // =========================================================================
  static void initialize();

  // =========================================================================
  // INSTALL — Register a new codex entry (Installer IV+ required)
  //
  // The codex sits in-in from this point forward. Binary sees it as
  // present. Other modules can reference it.
  // =========================================================================
  static int install_codex(const char* name, CodexShape shape, CodexColor color,
                           const char* functionality, RigorLevel rigor,
                           const char* improvement,
                           const char* code, size_t code_len,
                           InstallerGrade installer_grade,
                           const char* installer_name);

  // =========================================================================
  // REFERENCE — Look up a codex by name or ID
  // =========================================================================
  static CodexEntry* find_by_name(const char* name);
  static CodexEntry* find_by_id(int codex_id);

  // =========================================================================
  // QUERY — What does the codex tell neighboring modules?
  // =========================================================================
  static const char*  get_name(int codex_id);
  static size_t       get_size(int codex_id);
  static CodexShape   get_shape(int codex_id);
  static CodexColor   get_color(int codex_id);
  static const char*  get_code(int codex_id, InstallerGrade requester_grade);
  static const char*  get_functionality(int codex_id);
  static RigorLevel   get_rigor(int codex_id);
  static const char*  get_improvement(int codex_id);

  // =========================================================================
  // WITHDRAW — Remove from active duty (Installer IV+ only)
  // Does not delete — marks as withdrawn. Can be reactivated.
  // =========================================================================
  static bool withdraw_codex(int codex_id, InstallerGrade grade);
  static bool reactivate_codex(int codex_id, InstallerGrade grade);

  // =========================================================================
  // INVENTORY
  // =========================================================================
  static void print_registry(outputStream* st);
  static void print_entry(int codex_id, outputStream* st);
  static int  total_entries() { return _total_entries; }
  static int  active_entries();

  // =========================================================================
  // Diagnostics
  // =========================================================================
  static void print_status(outputStream* st);
  static const char* shape_name(CodexShape s);
  static const char* color_name(CodexColor c);
  static const char* rigor_name(RigorLevel r);
  static const char* grade_name(InstallerGrade g);
};

#endif // SHARE_RUNTIME_JVMCODEX_HPP
