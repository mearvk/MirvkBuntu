/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmCircuit.hpp - Observer Grade Circuit for Secure JVM
 *
 * Provides 2-3 observer circuits in addition to the main process circuit.
 * Authorized observers (inspectors, techs, forensic leads, attorneys, MDs,
 * presidential ensignia) can connect to a running JVM via secure shell or
 * telnet-like interface to observe, grade, and file reports.
 *
 * ARCHITECTURE:
 *
 *   ┌─────────────────────────────────────────────────────────┐
 *   │  MAIN CIRCUIT (Electron 0)                               │
 *   │  The running JVM — application threads, GC, JIT          │
 *   │  This is the production process.                         │
 *   └───────────────┬─────────────────────────────────────────┘
 *                   │ (read-only observation pipe)
 *   ┌───────────────┴─────────────────────────────────────────┐
 *   │  OBSERVER CIRCUIT 1 (Electron 1)                         │
 *   │  Software Inspectors, Lead Installer Techs               │
 *   │  Can: view class loads, memory, thread state, grades     │
 *   │  Cannot: mutate state, pause production                  │
 *   └───────────────┬─────────────────────────────────────────┘
 *                   │ (forwarded telemetry)
 *   ┌───────────────┴─────────────────────────────────────────┐
 *   │  OBSERVER CIRCUIT 2 (Electron 2)                         │
 *   │  Forensic Leads, Attorneys of Graves, MD Concerns        │
 *   │  Can: deep inspection, native frame view, file reports   │
 *   │  Can: request pause (requires Circuit 1 consent)         │
 *   └───────────────┬─────────────────────────────────────────┘
 *                   │ (presidential link)
 *   ┌───────────────┴─────────────────────────────────────────┐
 *   │  OBSERVER CIRCUIT 3 (Electron 3) — Presidential          │
 *   │  President's ensignia or course designation               │
 *   │  Can: circle one or more JVMs as carrier-grade           │
 *   │  Can: link JVMs for system purpose grading               │
 *   │  Can: order system breakdown after grading               │
 *   │  Can: file final report and archive                      │
 *   └─────────────────────────────────────────────────────────┘
 *
 * JVM LINKING:
 *   Multiple JVMs can be linked for system grading purpose:
 *   JVM-A ←→ JVM-B ←→ JVM-C (carrier chain)
 *   A Circuit 3 observer can see across the linked chain.
 *   After grading, the system may be broken down and report filed.
 *
 * ACCESS METHODS:
 *   - SSH to JVM (port configurable, default 2222)
 *     Authenticated via public key or certificate
 *   - Telnet to JVM (port configurable, default 2223)
 *     For legacy/simple access, restricted to localhost by default
 *   - Unix domain socket (/var/run/jvm-circuit-<pid>.sock)
 *     For local observer processes
 *
 * GRADING WORKFLOW:
 *   1. Observer connects to circuit (SSH/telnet/socket)
 *   2. Authenticates with credential + circuit level
 *   3. Observes JVM operation (classes, memory, threads, security)
 *   4. Assigns grade (A-F or numerical 0-100)
 *   5. Files report (stored in JVM + optionally forwarded)
 *   6. If linked: propagates grade to linked JVMs
 *   7. If Circuit 3: may order breakdown after grading
 */

#ifndef SHARE_RUNTIME_JVMCIRCUIT_HPP
#define SHARE_RUNTIME_JVMCIRCUIT_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"

// ============================================================================
// Circuit Levels
// ============================================================================

enum CircuitLevel {
  CIRCUIT_MAIN      = 0,  // The running JVM itself (not an observer)
  CIRCUIT_OBSERVER_1 = 1, // Software inspectors, lead installer techs
  CIRCUIT_OBSERVER_2 = 2, // Forensic leads, attorneys, MD concerns
  CIRCUIT_OBSERVER_3 = 3  // Presidential ensignia / course designation
};

// ============================================================================
// Observer Roles
// ============================================================================

enum ObserverRole {
  ROLE_SOFTWARE_INSPECTOR    = 0,
  ROLE_LEAD_INSTALLER_TECH   = 1,
  ROLE_FORENSIC_LEAD         = 2,
  ROLE_ATTORNEY_OF_GRAVES    = 3,
  ROLE_DOCTOR_MD_CONCERN     = 4,
  ROLE_PRESIDENTIAL_ENSIGNIA = 5,
  ROLE_SYSTEM_GRADER         = 6
};

// ============================================================================
// Grading
// ============================================================================

enum GradeLevel {
  GRADE_A = 0,  // Excellent — no concerns
  GRADE_B = 1,  // Good — minor observations
  GRADE_C = 2,  // Acceptable — noted concerns
  GRADE_D = 3,  // Deficient — requires attention
  GRADE_F = 4   // Failing — immediate action required
};

// ============================================================================
// JVM Link Status (for carrier chain)
// ============================================================================

enum LinkStatus {
  LINK_NONE        = 0,  // Standalone JVM
  LINK_ACTIVE      = 1,  // Linked and communicating
  LINK_GRADING     = 2,  // Currently being graded as a chain
  LINK_BREAKDOWN   = 3,  // Ordered to break down after grading
  LINK_ARCHIVED    = 4   // Grading complete, report filed
};

// ============================================================================
// Observer Session
// ============================================================================

struct ObserverSession {
  int             session_id;
  CircuitLevel    circuit;
  ObserverRole    role;
  char            identity[256];     // Observer name/credential
  char            authority[256];    // Issuing authority
  uint64_t        connect_time_ms;   // When connected
  bool            active;
  int             socket_fd;         // Connection file descriptor
  ObserverSession* next;
};

// ============================================================================
// Grading Report
// ============================================================================

struct GradingReport {
  int             report_id;
  int             session_id;        // Who filed it
  CircuitLevel    circuit;
  ObserverRole    role;
  char            observer_name[256];
  char            authority[256];
  GradeLevel      grade;
  int             numerical_score;   // 0-100
  char            summary[1024];     // Brief assessment
  char            findings[4096];    // Detailed findings
  uint64_t        filed_time_ms;
  bool            propagated;        // Sent to linked JVMs
  GradingReport*  next;
};

// ============================================================================
// Linked JVM Entry
// ============================================================================

struct LinkedJvm {
  char            host[256];         // Host address
  int             port;              // Circuit port
  char            jvm_id[128];       // Unique JVM identifier
  LinkStatus      status;
  GradeLevel      last_grade;
  uint64_t        link_time_ms;
  LinkedJvm*      next;
};

// ============================================================================
// Main Circuit Controller
// ============================================================================

class JvmCircuit : public AllStatic {
private:
  static bool             _enabled;
  static bool             _ssh_enabled;
  static bool             _telnet_enabled;
  static bool             _socket_enabled;
  static int              _ssh_port;
  static int              _telnet_port;
  static char             _socket_path[512];
  static char             _jvm_id[128];

  // Sessions
  static ObserverSession* _sessions_head;
  static int              _next_session_id;
  static int              _active_observers;
  static volatile int     _session_lock;

  // Reports
  static GradingReport*   _reports_head;
  static int              _next_report_id;
  static int              _total_reports;

  // Linked JVMs
  static LinkedJvm*       _links_head;
  static int              _link_count;
  static LinkStatus       _chain_status;

  // Locking
  static void lock();
  static void unlock();

  // Internal
  static bool authenticate_observer(const char* credential, CircuitLevel requested,
                                    ObserverRole* out_role, char* out_identity, size_t id_len);
  static void handle_observer_command(ObserverSession* session, const char* command,
                                      outputStream* response);
  static void send_telemetry_to_observers(CircuitLevel min_level, const char* event);
  static void propagate_grade_to_links(GradingReport* report);

  // Listener threads (SSH, telnet, socket)
  static void ssh_listener_thread(void* arg);
  static void telnet_listener_thread(void* arg);
  static void socket_listener_thread(void* arg);
  static void observer_handler_thread(void* arg);

public:
  // =========================================================================
  // Initialization
  // =========================================================================
  static void initialize();
  static void start_listeners();
  static void shutdown();

  // =========================================================================
  // Observer Connection
  // =========================================================================
  static ObserverSession* accept_observer(int fd, CircuitLevel circuit,
                                          const char* credential);
  static void disconnect_observer(ObserverSession* session);
  static int  active_observer_count() { return _active_observers; }

  // =========================================================================
  // Observation Events (called by JVM internals to notify observers)
  // =========================================================================
  static void notify_class_load(const char* class_name, int grade, bool has_native);
  static void notify_gc_event(const char* gc_name, size_t freed, size_t heap_used);
  static void notify_thread_event(const char* thread_name, const char* event);
  static void notify_security_event(const char* source, const char* detail);
  static void notify_integrity_violation(const char* description);

  // =========================================================================
  // Grading
  // =========================================================================
  static int  file_report(ObserverSession* session, GradeLevel grade,
                          int numerical_score, const char* summary, const char* findings);
  static void print_reports(outputStream* st);
  static GradeLevel overall_grade();

  // =========================================================================
  // JVM Linking (carrier chain)
  // =========================================================================
  static bool link_jvm(const char* host, int port, const char* jvm_id);
  static void unlink_jvm(const char* jvm_id);
  static void unlink_all();
  static void grade_chain(CircuitLevel authority);  // Grade all linked JVMs
  static void breakdown_chain();                    // Order breakdown after grading
  static void print_chain(outputStream* st);

  // =========================================================================
  // Commands available to observers (via SSH/telnet/socket)
  // =========================================================================
  //
  // Circuit 1 commands:
  //   status          - JVM status overview
  //   classes         - List loaded classes (last N)
  //   memory          - Heap/non-heap usage
  //   threads         - Thread listing
  //   guard           - ClassLoadGuard stats
  //   integrity       - JvmIntegrity status
  //   history [N]     - Class load history
  //
  // Circuit 2 commands (in addition to Circuit 1):
  //   inspect <class> - Full technical frame
  //   native <class>  - Native method view
  //   reports         - Filed grading reports
  //   grade <A-F> <score> <summary> - File a grading report
  //   pause           - Request JVM pause (needs Circuit 1 consent)
  //   resume          - Resume from pause
  //
  // Circuit 3 commands (in addition to Circuit 1+2):
  //   link <host:port>    - Link another JVM to this chain
  //   unlink <jvm_id>     - Remove JVM from chain
  //   chain               - Show linked JVM chain
  //   grade-chain         - Grade entire carrier chain
  //   breakdown           - Order system breakdown (after grading)
  //   archive             - Archive all reports and finalize
  //   circle              - Mark this JVM as circled by presidential ensignia
  //

  // =========================================================================
  // Diagnostics
  // =========================================================================
  static void print_status(outputStream* st);
  static void print_config(outputStream* st);
  static const char* circuit_name(CircuitLevel level);
  static const char* role_name(ObserverRole role);
  static const char* grade_name(GradeLevel grade);
  static const char* link_status_name(LinkStatus status);
};

#endif // SHARE_RUNTIME_JVMCIRCUIT_HPP
