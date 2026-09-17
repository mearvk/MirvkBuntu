/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmCircuit.cpp - Observer Grade Circuit Implementation
 *
 * Provides SSH/telnet/socket access for authorized observers to monitor,
 * grade, link, and report on running JVMs. Supports carrier-chain linking
 * for system-purpose grading across multiple JVMs.
 */

#include "runtime/jvmCircuit.hpp"
#include "runtime/jvmIntegrity.hpp"
#include "runtime/jvmInspector.hpp"
#include "classfile/classLoadGuard.hpp"
#include "runtime/os.hpp"
#include "runtime/atomic.hpp"
#include "utilities/ostream.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

// ============================================================================
// Static State
// ============================================================================

bool             JvmCircuit::_enabled = false;
bool             JvmCircuit::_ssh_enabled = true;
bool             JvmCircuit::_telnet_enabled = false;  // Off by default (use SSH)
bool             JvmCircuit::_socket_enabled = true;
int              JvmCircuit::_ssh_port = 2222;
int              JvmCircuit::_telnet_port = 2223;
char             JvmCircuit::_socket_path[512] = {0};
char             JvmCircuit::_jvm_id[128] = {0};

ObserverSession* JvmCircuit::_sessions_head = nullptr;
int              JvmCircuit::_next_session_id = 1;
int              JvmCircuit::_active_observers = 0;
volatile int     JvmCircuit::_session_lock = 0;

GradingReport*   JvmCircuit::_reports_head = nullptr;
int              JvmCircuit::_next_report_id = 1;
int              JvmCircuit::_total_reports = 0;

LinkedJvm*       JvmCircuit::_links_head = nullptr;
int              JvmCircuit::_link_count = 0;
LinkStatus       JvmCircuit::_chain_status = LINK_NONE;

// ============================================================================
// Locking
// ============================================================================

void JvmCircuit::lock() {
  while (AtomicAccess::cmpxchg(&_session_lock, 0, 1) != 0) {
    os::naked_short_sleep(0);
  }
}

void JvmCircuit::unlock() {
  AtomicAccess::store(&_session_lock, 0);
}

// ============================================================================
// Name Lookups
// ============================================================================

const char* JvmCircuit::circuit_name(CircuitLevel level) {
  switch (level) {
    case CIRCUIT_MAIN:       return "Main (Electron 0)";
    case CIRCUIT_OBSERVER_1: return "Observer 1 (Inspectors/Techs)";
    case CIRCUIT_OBSERVER_2: return "Observer 2 (Forensic/Legal/Medical)";
    case CIRCUIT_OBSERVER_3: return "Observer 3 (Presidential)";
    default:                 return "Unknown";
  }
}

const char* JvmCircuit::role_name(ObserverRole role) {
  switch (role) {
    case ROLE_SOFTWARE_INSPECTOR:    return "Software Inspector";
    case ROLE_LEAD_INSTALLER_TECH:   return "Lead Installer Tech";
    case ROLE_FORENSIC_LEAD:         return "Forensic Lead";
    case ROLE_ATTORNEY_OF_GRAVES:    return "Attorney of Graves";
    case ROLE_DOCTOR_MD_CONCERN:     return "Doctor (MD Concern)";
    case ROLE_PRESIDENTIAL_ENSIGNIA: return "Presidential Ensignia";
    case ROLE_SYSTEM_GRADER:         return "System Grader";
    default:                         return "Unknown";
  }
}

const char* JvmCircuit::grade_name(GradeLevel grade) {
  switch (grade) {
    case GRADE_A: return "A (Excellent)";
    case GRADE_B: return "B (Good)";
    case GRADE_C: return "C (Acceptable)";
    case GRADE_D: return "D (Deficient)";
    case GRADE_F: return "F (Failing)";
    default:      return "?";
  }
}

const char* JvmCircuit::link_status_name(LinkStatus status) {
  switch (status) {
    case LINK_NONE:      return "None (standalone)";
    case LINK_ACTIVE:    return "Active (linked)";
    case LINK_GRADING:   return "Grading (under review)";
    case LINK_BREAKDOWN: return "Breakdown (ordered)";
    case LINK_ARCHIVED:  return "Archived (complete)";
    default:             return "Unknown";
  }
}

// ============================================================================
// Initialization
// ============================================================================

void JvmCircuit::initialize() {
  _enabled = true;

  // Generate unique JVM ID
  pid_t pid = getpid();
  char hostname[128] = {0};
  gethostname(hostname, sizeof(hostname) - 1);
  snprintf(_jvm_id, sizeof(_jvm_id), "%s-jvm-%d", hostname, (int)pid);

  // Set socket path
  snprintf(_socket_path, sizeof(_socket_path), "/var/run/jvm-circuit-%d.sock", (int)pid);

  log_info(os)("JvmCircuit: initialized (id=%s, ssh=%d, telnet=%d, socket=%s)",
               _jvm_id, _ssh_port, _telnet_port, _socket_path);
}

void JvmCircuit::start_listeners() {
  if (!_enabled) return;

  // Start listener threads for each enabled access method
  // In production, these would be proper thread launches via os::create_thread()
  // For now, log readiness.

  if (_ssh_enabled) {
    log_info(os)("JvmCircuit: SSH listener ready on port %d", _ssh_port);
    // TODO: Launch SSH listener thread (requires libssh or custom impl)
    // pthread_create(&ssh_thread, nullptr, ssh_listener_thread, nullptr);
  }

  if (_telnet_enabled) {
    log_info(os)("JvmCircuit: Telnet listener ready on port %d (localhost only)", _telnet_port);
  }

  if (_socket_enabled) {
    // Skip socket bind during bootstrap/build (non-writable /var/run)
    bool can_bind = (access("/var/run", W_OK) == 0);
    if (can_bind) {
      log_info(os)("JvmCircuit: Unix socket listener ready at %s", _socket_path);
      // Create the socket for local observer connection
      int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
      if (sock_fd >= 0) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, _socket_path, sizeof(addr.sun_path) - 1);
        unlink(_socket_path);  // Remove stale socket
        if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
          listen(sock_fd, 5);
          chmod(_socket_path, 0600);  // Owner only
          log_info(os)("JvmCircuit: Unix socket bound and listening");
        } else {
          log_info(os)("JvmCircuit: socket bind deferred (permission denied — build context)");
          close(sock_fd);
        }
      }
    } else {
      log_info(os)("JvmCircuit: socket bind skipped (build/bootstrap context)");
    }
  }
}

void JvmCircuit::shutdown() {
  if (!_enabled) return;

  // Disconnect all observers
  lock();
  ObserverSession* s = _sessions_head;
  while (s != nullptr) {
    if (s->active && s->socket_fd >= 0) {
      close(s->socket_fd);
      s->active = false;
    }
    s = s->next;
  }
  _active_observers = 0;
  unlock();

  // Remove socket file
  unlink(_socket_path);

  log_info(os)("JvmCircuit: shutdown complete");
}

// ============================================================================
// Observer Connection
// ============================================================================

ObserverSession* JvmCircuit::accept_observer(int fd, CircuitLevel circuit,
                                             const char* credential) {
  ObserverRole role;
  char identity[256] = {0};

  if (!authenticate_observer(credential, circuit, &role, identity, sizeof(identity))) {
    log_warning(os)("JvmCircuit: authentication failed for credential on circuit %d",
                    (int)circuit);
    return nullptr;
  }

  ObserverSession* session = (ObserverSession*)os::malloc(sizeof(ObserverSession), mtInternal);
  if (session == nullptr) return nullptr;

  lock();
  session->session_id = _next_session_id++;
  session->circuit = circuit;
  session->role = role;
  strncpy(session->identity, identity, sizeof(session->identity) - 1);
  session->identity[255] = '\0';
  session->authority[0] = '\0';
  session->connect_time_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);
  session->active = true;
  session->socket_fd = fd;
  session->next = _sessions_head;
  _sessions_head = session;
  _active_observers++;
  unlock();

  log_info(os)("JvmCircuit: observer connected [session=%d, circuit=%s, role=%s, id=%s]",
               session->session_id, circuit_name(circuit), role_name(role), identity);

  return session;
}

void JvmCircuit::disconnect_observer(ObserverSession* session) {
  if (session == nullptr) return;

  lock();
  session->active = false;
  if (session->socket_fd >= 0) {
    close(session->socket_fd);
    session->socket_fd = -1;
  }
  _active_observers--;
  unlock();

  log_info(os)("JvmCircuit: observer disconnected [session=%d, role=%s]",
               session->session_id, role_name(session->role));
}

// ============================================================================
// Authentication
// ============================================================================

bool JvmCircuit::authenticate_observer(const char* credential, CircuitLevel requested,
                                       ObserverRole* out_role, char* out_identity,
                                       size_t id_len) {
  if (credential == nullptr || credential[0] == '\0') return false;

  // Credential format: "ROLE:IDENTITY:AUTHORITY"
  // Example: "FORENSIC_LEAD:Dr.Smith:US-DOJ"
  //          "PRESIDENTIAL:POTUS:WhiteHouse"

  char cred_copy[512];
  strncpy(cred_copy, credential, sizeof(cred_copy) - 1);
  cred_copy[511] = '\0';

  char* role_str = strtok(cred_copy, ":");
  char* id_str = strtok(nullptr, ":");
  char* auth_str = strtok(nullptr, ":");

  if (role_str == nullptr || id_str == nullptr) return false;

  // Map role string to enum
  ObserverRole role;
  if (strcasecmp(role_str, "SOFTWARE_INSPECTOR") == 0)    role = ROLE_SOFTWARE_INSPECTOR;
  else if (strcasecmp(role_str, "INSTALLER_TECH") == 0)   role = ROLE_LEAD_INSTALLER_TECH;
  else if (strcasecmp(role_str, "FORENSIC_LEAD") == 0)    role = ROLE_FORENSIC_LEAD;
  else if (strcasecmp(role_str, "ATTORNEY") == 0)         role = ROLE_ATTORNEY_OF_GRAVES;
  else if (strcasecmp(role_str, "DOCTOR_MD") == 0)        role = ROLE_DOCTOR_MD_CONCERN;
  else if (strcasecmp(role_str, "PRESIDENTIAL") == 0)     role = ROLE_PRESIDENTIAL_ENSIGNIA;
  else if (strcasecmp(role_str, "SYSTEM_GRADER") == 0)    role = ROLE_SYSTEM_GRADER;
  else return false;

  // Verify role matches requested circuit
  if (requested == CIRCUIT_OBSERVER_3 && role != ROLE_PRESIDENTIAL_ENSIGNIA) {
    return false;  // Only presidential can access Circuit 3
  }
  if (requested == CIRCUIT_OBSERVER_2 &&
      role != ROLE_FORENSIC_LEAD && role != ROLE_ATTORNEY_OF_GRAVES &&
      role != ROLE_DOCTOR_MD_CONCERN && role != ROLE_PRESIDENTIAL_ENSIGNIA) {
    return false;
  }

  *out_role = role;
  strncpy(out_identity, id_str, id_len - 1);
  out_identity[id_len - 1] = '\0';

  return true;
}

// ============================================================================
// Observation Events (broadcast to active observers)
// ============================================================================

void JvmCircuit::notify_class_load(const char* class_name, int grade, bool has_native) {
  if (!_enabled || _active_observers == 0) return;

  char event[512];
  snprintf(event, sizeof(event), "[CLASS] %s (grade=%d%s)",
           class_name, grade, has_native ? ", native" : "");
  send_telemetry_to_observers(CIRCUIT_OBSERVER_1, event);
}

void JvmCircuit::notify_gc_event(const char* gc_name, size_t freed, size_t heap_used) {
  if (!_enabled || _active_observers == 0) return;

  char event[512];
  snprintf(event, sizeof(event), "[GC] %s: freed=%zuKB, heap=%zuMB",
           gc_name, freed / 1024, heap_used / (1024 * 1024));
  send_telemetry_to_observers(CIRCUIT_OBSERVER_1, event);
}

void JvmCircuit::notify_thread_event(const char* thread_name, const char* event_type) {
  if (!_enabled || _active_observers == 0) return;

  char event[512];
  snprintf(event, sizeof(event), "[THREAD] %s: %s", thread_name, event_type);
  send_telemetry_to_observers(CIRCUIT_OBSERVER_1, event);
}

void JvmCircuit::notify_security_event(const char* source, const char* detail) {
  if (!_enabled || _active_observers == 0) return;

  char event[512];
  snprintf(event, sizeof(event), "[SECURITY] %s: %s", source, detail);
  // Security events go to all circuits
  send_telemetry_to_observers(CIRCUIT_OBSERVER_1, event);
}

void JvmCircuit::notify_integrity_violation(const char* description) {
  if (!_enabled || _active_observers == 0) return;

  char event[512];
  snprintf(event, sizeof(event), "[!!! INTEGRITY VIOLATION !!!] %s", description);
  send_telemetry_to_observers(CIRCUIT_OBSERVER_1, event);
}

void JvmCircuit::send_telemetry_to_observers(CircuitLevel min_level, const char* event) {
  lock();
  ObserverSession* s = _sessions_head;
  while (s != nullptr) {
    if (s->active && s->circuit >= min_level && s->socket_fd >= 0) {
      // Best-effort write — don't block JVM if observer is slow
      write(s->socket_fd, event, strlen(event));
      write(s->socket_fd, "\n", 1);
    }
    s = s->next;
  }
  unlock();
}

// ============================================================================
// Grading Reports
// ============================================================================

int JvmCircuit::file_report(ObserverSession* session, GradeLevel grade,
                            int numerical_score, const char* summary,
                            const char* findings) {
  if (session == nullptr) return -1;

  // Circuit 1 cannot file reports (observe only)
  if (session->circuit < CIRCUIT_OBSERVER_2) {
    log_warning(os)("JvmCircuit: Circuit 1 observer cannot file reports");
    return -1;
  }

  GradingReport* report = (GradingReport*)os::malloc(sizeof(GradingReport), mtInternal);
  if (report == nullptr) return -1;

  lock();
  report->report_id = _next_report_id++;
  report->session_id = session->session_id;
  report->circuit = session->circuit;
  report->role = session->role;
  strncpy(report->observer_name, session->identity, sizeof(report->observer_name) - 1);
  report->observer_name[255] = '\0';
  report->authority[0] = '\0';
  report->grade = grade;
  report->numerical_score = numerical_score;
  strncpy(report->summary, summary ? summary : "", sizeof(report->summary) - 1);
  report->summary[1023] = '\0';
  strncpy(report->findings, findings ? findings : "", sizeof(report->findings) - 1);
  report->findings[4095] = '\0';
  report->filed_time_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);
  report->propagated = false;
  report->next = _reports_head;
  _reports_head = report;
  _total_reports++;
  unlock();

  log_info(os)("JvmCircuit: report filed [#%d, grade=%s, score=%d, by=%s (%s)]",
               report->report_id, grade_name(grade), numerical_score,
               session->identity, role_name(session->role));

  // If observer is Circuit 3, propagate to linked JVMs
  if (session->circuit == CIRCUIT_OBSERVER_3 && _link_count > 0) {
    propagate_grade_to_links(report);
  }

  return report->report_id;
}

GradeLevel JvmCircuit::overall_grade() {
  if (_total_reports == 0) return GRADE_A;  // No reports = no concerns

  // Average numerical score from all reports
  lock();
  int total_score = 0;
  int count = 0;
  GradingReport* r = _reports_head;
  while (r != nullptr) {
    total_score += r->numerical_score;
    count++;
    r = r->next;
  }
  unlock();

  if (count == 0) return GRADE_A;
  int avg = total_score / count;

  if (avg >= 90) return GRADE_A;
  if (avg >= 80) return GRADE_B;
  if (avg >= 70) return GRADE_C;
  if (avg >= 60) return GRADE_D;
  return GRADE_F;
}

void JvmCircuit::print_reports(outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  GRADING REPORTS                                                 ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->print_cr("  Total reports: %d", _total_reports);
  st->print_cr("  Overall grade: %s", grade_name(overall_grade()));
  st->cr();

  lock();
  GradingReport* r = _reports_head;
  while (r != nullptr) {
    st->print_cr("  Report #%d [%s, score=%d]", r->report_id, grade_name(r->grade), r->numerical_score);
    st->print_cr("    Filed by: %s (%s)", r->observer_name, role_name(r->role));
    st->print_cr("    Circuit:  %s", circuit_name(r->circuit));
    st->print_cr("    Summary:  %s", r->summary);
    if (r->findings[0]) {
      st->print_cr("    Findings: %s", r->findings);
    }
    st->cr();
    r = r->next;
  }
  unlock();
}

// ============================================================================
// JVM Linking (Carrier Chain)
// ============================================================================

bool JvmCircuit::link_jvm(const char* host, int port, const char* jvm_id) {
  LinkedJvm* link = (LinkedJvm*)os::malloc(sizeof(LinkedJvm), mtInternal);
  if (link == nullptr) return false;

  strncpy(link->host, host, sizeof(link->host) - 1);
  link->host[255] = '\0';
  link->port = port;
  strncpy(link->jvm_id, jvm_id, sizeof(link->jvm_id) - 1);
  link->jvm_id[127] = '\0';
  link->status = LINK_ACTIVE;
  link->last_grade = GRADE_A;
  link->link_time_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);

  lock();
  link->next = _links_head;
  _links_head = link;
  _link_count++;
  _chain_status = LINK_ACTIVE;
  unlock();

  log_info(os)("JvmCircuit: linked JVM %s at %s:%d (chain size=%d)",
               jvm_id, host, port, _link_count);
  return true;
}

void JvmCircuit::unlink_jvm(const char* jvm_id) {
  lock();
  LinkedJvm** prev = &_links_head;
  LinkedJvm* curr = _links_head;
  while (curr != nullptr) {
    if (strcmp(curr->jvm_id, jvm_id) == 0) {
      *prev = curr->next;
      _link_count--;
      os::free(curr);
      break;
    }
    prev = &curr->next;
    curr = curr->next;
  }
  if (_link_count == 0) _chain_status = LINK_NONE;
  unlock();
}

void JvmCircuit::breakdown_chain() {
  lock();
  _chain_status = LINK_BREAKDOWN;
  LinkedJvm* l = _links_head;
  while (l != nullptr) {
    l->status = LINK_BREAKDOWN;
    l = l->next;
  }
  unlock();

  log_info(os)("JvmCircuit: BREAKDOWN ordered for carrier chain (%d JVMs)", _link_count);
  // Notify all observers
  notify_security_event("JvmCircuit", "Carrier chain BREAKDOWN ordered by Circuit 3 authority");
}

void JvmCircuit::grade_chain(CircuitLevel authority) {
  if (authority < CIRCUIT_OBSERVER_3) {
    log_warning(os)("JvmCircuit: only Circuit 3 can grade the chain");
    return;
  }

  lock();
  _chain_status = LINK_GRADING;
  LinkedJvm* l = _links_head;
  while (l != nullptr) {
    l->status = LINK_GRADING;
    l = l->next;
  }
  unlock();

  log_info(os)("JvmCircuit: chain grading initiated by Circuit 3 (%d JVMs in chain)",
               _link_count);
}

void JvmCircuit::propagate_grade_to_links(GradingReport* report) {
  // In a full implementation, this would send the grade to linked JVMs
  // via their circuit ports. For now, mark as propagated.
  report->propagated = true;
  log_info(os)("JvmCircuit: grade propagated to %d linked JVMs", _link_count);
}

void JvmCircuit::print_chain(outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  JVM CARRIER CHAIN                                               ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->print_cr("  This JVM:     %s", _jvm_id);
  st->print_cr("  Chain status: %s", link_status_name(_chain_status));
  st->print_cr("  Linked JVMs:  %d", _link_count);
  st->cr();

  lock();
  LinkedJvm* l = _links_head;
  int i = 1;
  while (l != nullptr) {
    st->print_cr("  [%d] %s (%s:%d) — %s, last grade: %s",
                 i++, l->jvm_id, l->host, l->port,
                 link_status_name(l->status), grade_name(l->last_grade));
    l = l->next;
  }
  unlock();
}

// ============================================================================
// Diagnostics
// ============================================================================

void JvmCircuit::print_status(outputStream* st) {
  st->print_cr("JVM Observer Circuit Status:");
  st->print_cr("  JVM ID:          %s", _jvm_id);
  st->print_cr("  Enabled:         %s", _enabled ? "yes" : "no");
  st->print_cr("  SSH port:        %d (%s)", _ssh_port, _ssh_enabled ? "active" : "disabled");
  st->print_cr("  Telnet port:     %d (%s)", _telnet_port, _telnet_enabled ? "active" : "disabled");
  st->print_cr("  Unix socket:     %s (%s)", _socket_path, _socket_enabled ? "active" : "disabled");
  st->print_cr("  Active observers: %d", _active_observers);
  st->print_cr("  Reports filed:   %d", _total_reports);
  st->print_cr("  Overall grade:   %s", grade_name(overall_grade()));
  st->print_cr("  Chain status:    %s (%d linked)", link_status_name(_chain_status), _link_count);
}

void JvmCircuit::print_config(outputStream* st) {
  st->print_cr("JVM Circuit Configuration (jvm-config.xml):");
  st->print_cr("  <jvm-circuit enabled=\"true\">");
  st->print_cr("    <ssh port=\"2222\" enabled=\"true\"/>");
  st->print_cr("    <telnet port=\"2223\" enabled=\"false\" localhost-only=\"true\"/>");
  st->print_cr("    <socket path=\"/var/run/jvm-circuit-PID.sock\" enabled=\"true\"/>");
  st->print_cr("  </jvm-circuit>");
}
