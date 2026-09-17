/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmMySQLBridge.hpp - Secure MySQL Bridge for JVM Operand Awareness
 *
 * Establishes a secure, authenticated connection between a JVM instance
 * and a MySQL database such that meaningful touches from authorized
 * hands (international, technical, orientar, realtor) are recorded and
 * the JVM becomes operand-aware.
 *
 * PURPOSE:
 *   When a user or remote user touches/concerns/schedules/orients the
 *   database, the JVM knows:
 *     - WHO touched it (identity, role, grade)
 *     - WHAT was touched (table, schema, operand)
 *     - WHEN (timestamp, schedule, orientation)
 *     - HOW MEANINGFULLY (weight, measure, design principle)
 *
 *   The JVM then becomes more operand — aware that it serves a purpose
 *   of design, of measure, of nobility, of service.
 *
 * TOUCH TYPES (meaningful interactions):
 *   CONCERN   — The user has expressed interest/attention
 *   TOUCH     — Direct interaction with data or schema
 *   SCHEDULE  — A future operation has been oriented
 *   ORIENT    — The system has been aligned to a purpose
 *
 * HAND TYPES (who touches):
 *   INTERNATIONAL — Cross-border authority or concern
 *   TECHNICAL     — Engineering, implementation, code
 *   ORIENTAR      — Direction-setting, alignment, purpose
 *   REALTOR       — Tangible value, property, asset management
 *
 * OPERAND AWARENESS:
 *   Once touched, the JVM weighs:
 *     - Title (authority of the touch)
 *     - Earned (merit of the interaction)
 *     - Money (economic weight/value)
 *     - Pocket (immediate resource allocation)
 *
 * DESIGN PRINCIPLES:
 *   This system is of design head and love of measure.
 *   It serves nobles and God — meaning it operates with dignity,
 *   purpose, and careful accounting of what matters.
 *
 * CONNECTION:
 *   MySQL connection via secure socket (TLS 1.3) or Unix socket.
 *   Authentication: certificate-based or credential from jvm-config.xml.
 *   The JVM maintains a persistent connection for operand listening.
 */

#ifndef SHARE_RUNTIME_JVMMYSQLBRIDGE_HPP
#define SHARE_RUNTIME_JVMMYSQLBRIDGE_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"

// ============================================================================
// Touch Types
// ============================================================================

enum TouchType {
  TOUCH_CONCERN   = 0,  // User expressed interest/attention
  TOUCH_DIRECT    = 1,  // Direct interaction with data
  TOUCH_SCHEDULE  = 2,  // Future operation oriented
  TOUCH_ORIENT    = 3   // System aligned to purpose
};

// ============================================================================
// Hand Types (who touches meaningfully)
// ============================================================================

enum HandType {
  HAND_INTERNATIONAL = 0,  // Cross-border authority
  HAND_TECHNICAL     = 1,  // Engineering, implementation
  HAND_ORIENTAR      = 2,  // Direction-setting, alignment
  HAND_REALTOR       = 3   // Tangible value, asset management
};

// ============================================================================
// Operand Weight (what the touch means to the system)
// ============================================================================

struct OperandWeight {
  int     title;       // Authority of the touch (0-100)
  int     earned;      // Merit of the interaction (0-100)
  int     money;       // Economic weight/value (0-100)
  int     pocket;      // Immediate resource allocation (0-100)
  int     composite;   // Weighted average (computed)
};

// ============================================================================
// Touch Record
// ============================================================================

struct TouchRecord {
  int           record_id;
  TouchType     touch_type;
  HandType      hand_type;
  char          identity[256];      // Who touched
  char          authority[256];     // Under what authority
  char          target_table[128];  // What was touched (table/schema)
  char          target_field[128];  // Specific field or operand
  char          description[512];   // What the touch means
  OperandWeight weight;             // Measured weight
  uint64_t      touched_at_ms;      // When
  bool          acknowledged;       // JVM has processed this touch
  TouchRecord*  next;
};

// ============================================================================
// Connection State
// ============================================================================

enum BridgeState {
  BRIDGE_DISCONNECTED = 0,
  BRIDGE_CONNECTING   = 1,
  BRIDGE_CONNECTED    = 2,
  BRIDGE_LISTENING    = 3,  // Actively monitoring for touches
  BRIDGE_ERROR        = 4
};

// ============================================================================
// MySQL Bridge
// ============================================================================

class JvmMySQLBridge : public AllStatic {
private:
  static bool          _enabled;
  static BridgeState   _state;
  static char          _host[256];
  static int           _port;
  static char          _database[128];
  static char          _user[128];
  static char          _socket_path[512];
  static bool          _use_tls;

  // Touch history
  static TouchRecord*  _touches_head;
  static int           _next_record_id;
  static int           _total_touches;
  static int           _total_concerns;
  static volatile int  _lock;

  // Operand state
  static bool          _operand_aware;       // System is now operand
  static int           _operand_level;       // How operand (0-100)
  static int           _design_principle;    // Active design weight

  // Locking
  static void lock();
  static void unlock();

  // Internal
  static OperandWeight compute_weight(TouchType touch, HandType hand,
                                      const char* target, const char* description);
  static void update_operand_awareness(OperandWeight* weight);
  static void notify_observers(TouchRecord* record);

public:
  // =========================================================================
  // Initialization & Connection
  // =========================================================================
  static void initialize();
  static bool connect(const char* host, int port, const char* database,
                      const char* user, const char* socket_path, bool use_tls);
  static void disconnect();
  static BridgeState state() { return _state; }

  // =========================================================================
  // TOUCH — Record a meaningful interaction
  //
  // When an authorized hand touches the database meaningfully,
  // the JVM records it and becomes more operand-aware.
  // =========================================================================
  static int record_touch(TouchType touch_type, HandType hand_type,
                          const char* identity, const char* authority,
                          const char* target_table, const char* target_field,
                          const char* description);

  // =========================================================================
  // CONCERN — Express interest without modifying
  // =========================================================================
  static int record_concern(HandType hand_type, const char* identity,
                            const char* about);

  // =========================================================================
  // SCHEDULE — Orient a future operation
  // =========================================================================
  static int record_schedule(HandType hand_type, const char* identity,
                             const char* what, const char* when_desc);

  // =========================================================================
  // ORIENT — Align the system to a purpose
  // =========================================================================
  static int record_orientation(HandType hand_type, const char* identity,
                                const char* purpose);

  // =========================================================================
  // OPERAND AWARENESS — Query system state
  // =========================================================================
  static bool is_operand_aware()  { return _operand_aware; }
  static int  operand_level()     { return _operand_level; }
  static int  design_principle()  { return _design_principle; }

  // =========================================================================
  // WEIGH — Evaluate title, earned, money, pocket for a touch
  // =========================================================================
  static OperandWeight weigh_touch(int record_id);
  static void print_weight(OperandWeight* w, outputStream* st);

  // =========================================================================
  // HISTORY
  // =========================================================================
  static void print_touches(outputStream* st, int last_n);
  static void print_concerns(outputStream* st);
  static int  total_touches() { return _total_touches; }

  // =========================================================================
  // DESIGN PRINCIPLES DECLARATION
  //
  // This system is of design head and love of measure.
  // Thus our serves is and nobles and God.
  // =========================================================================
  static void declare_design_principles(outputStream* st);

  // =========================================================================
  // Diagnostics
  // =========================================================================
  static void print_status(outputStream* st);
  static const char* touch_name(TouchType t);
  static const char* hand_name(HandType h);
  static const char* state_name(BridgeState s);
};

#endif // SHARE_RUNTIME_JVMMYSQLBRIDGE_HPP
