/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmResourceLoader.cpp - Secure Resource Loader Implementation
 *
 * Careful appropriation of C, S, HPP, JSON, and XML files into the JVM
 * with permission grading, content validation, and inventory tracking.
 */

#include "runtime/jvmResourceLoader.hpp"
#include "runtime/os.hpp"
#include "runtime/atomic.hpp"
#include "utilities/ostream.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Maximum file sizes per type
#define MAX_C_SIZE      (2 * 1024 * 1024)    // 2MB
#define MAX_ASM_SIZE    (1 * 1024 * 1024)    // 1MB
#define MAX_HPP_SIZE    (1 * 1024 * 1024)    // 1MB
#define MAX_JSON_SIZE   (10 * 1024 * 1024)   // 10MB
#define MAX_XML_SIZE    (10 * 1024 * 1024)   // 10MB

// ============================================================================
// Static State
// ============================================================================

bool             JvmResourceLoader::_enabled = false;
LoadedResource*  JvmResourceLoader::_inventory_head = nullptr;
int              JvmResourceLoader::_next_resource_id = 1;
int              JvmResourceLoader::_total_loaded = 0;
int              JvmResourceLoader::_total_rejected = 0;
volatile int     JvmResourceLoader::_lock = 0;

// ============================================================================
// Locking
// ============================================================================

void JvmResourceLoader::lock() {
  while (AtomicAccess::cmpxchg(&_lock, 0, 1) != 0) {
    os::naked_short_sleep(0);
  }
}

void JvmResourceLoader::unlock() {
  AtomicAccess::store(&_lock, 0);
}

// ============================================================================
// Initialization
// ============================================================================

void JvmResourceLoader::initialize() {
  _enabled = true;
  log_info(os)("JvmResourceLoader: initialized (C, S, HPP, JSON, XML loading with permission gates)");
}

// ============================================================================
// Name Lookups
// ============================================================================

const char* JvmResourceLoader::type_name(ResourceType type) {
  switch (type) {
    case RESOURCE_C:      return "C Source";
    case RESOURCE_ASM:    return "Assembly";
    case RESOURCE_HPP:    return "C++ Header";
    case RESOURCE_JSON:   return "JSON";
    case RESOURCE_XML:    return "XML";
    default:              return "Unknown";
  }
}

const char* JvmResourceLoader::type_extension(ResourceType type) {
  switch (type) {
    case RESOURCE_C:      return ".c";
    case RESOURCE_ASM:    return ".S";
    case RESOURCE_HPP:    return ".hpp";
    case RESOURCE_JSON:   return ".json";
    case RESOURCE_XML:    return ".xml";
    default:              return "?";
  }
}

const char* JvmResourceLoader::grade_name(LoaderGrade grade) {
  switch (grade) {
    case LOADER_GRADE_APPLICATION: return "Application (1)";
    case LOADER_GRADE_TRUSTED:     return "Trusted (2)";
    case LOADER_GRADE_SYSTEM:      return "System (3)";
    case LOADER_GRADE_KERNEL:      return "Kernel (4)";
    default:                       return "Unknown";
  }
}

const char* JvmResourceLoader::status_name(ContentStatus status) {
  switch (status) {
    case CONTENT_PENDING:     return "Pending";
    case CONTENT_VALIDATED:   return "Validated";
    case CONTENT_QUARANTINED: return "Quarantined";
    case CONTENT_ACTIVE:      return "Active";
    case CONTENT_REJECTED:    return "Rejected";
    default:                  return "Unknown";
  }
}

// ============================================================================
// Type Detection
// ============================================================================

ResourceType JvmResourceLoader::detect_type(const char* path) {
  if (path == nullptr) return RESOURCE_UNKNOWN;

  size_t len = strlen(path);
  if (len < 3) return RESOURCE_UNKNOWN;

  // Check extension
  const char* dot = strrchr(path, '.');
  if (dot == nullptr) return RESOURCE_UNKNOWN;

  if (strcasecmp(dot, ".c") == 0)    return RESOURCE_C;
  if (strcasecmp(dot, ".S") == 0 || strcasecmp(dot, ".s") == 0) return RESOURCE_ASM;
  if (strcasecmp(dot, ".hpp") == 0 || strcasecmp(dot, ".h") == 0) return RESOURCE_HPP;
  if (strcasecmp(dot, ".json") == 0) return RESOURCE_JSON;
  if (strcasecmp(dot, ".xml") == 0)  return RESOURCE_XML;

  return RESOURCE_UNKNOWN;
}

// ============================================================================
// Permission Check
// ============================================================================

bool JvmResourceLoader::check_permission(ResourceType type, LoaderGrade grade) {
  switch (type) {
    case RESOURCE_JSON:
    case RESOURCE_XML:
      // Grade 1+ can load data files
      return grade >= LOADER_GRADE_APPLICATION;

    case RESOURCE_HPP:
      // Grade 2+ can load headers (for inspection/reference)
      return grade >= LOADER_GRADE_TRUSTED;

    case RESOURCE_C:
    case RESOURCE_ASM:
      // Grade 3+ can load source code (serious business)
      return grade >= LOADER_GRADE_SYSTEM;

    default:
      return grade >= LOADER_GRADE_KERNEL;
  }
}

// ============================================================================
// Content Validation — C Source
// ============================================================================

bool JvmResourceLoader::validate_c_source(const char* content, size_t len,
                                           ValidationResult* result) {
  result->passed = true;
  result->concern_count = 0;

  // Banned patterns in C source (security concerns)
  static const char* banned_calls[] = {
    "exec(",  "execv(",  "execve(", "execvp(",
    "system(", "popen(",
    "fork(",   "vfork(",
    "dlopen(", "dlsym(",
    "ptrace(",
    "mprotect(",  // Could make code executable
    nullptr
  };

  static const char* banned_includes[] = {
    "#include <dlfcn.h>",      // Dynamic loading
    "#include <sys/ptrace.h>", // Process tracing
    nullptr
  };

  // Check for banned function calls
  for (int i = 0; banned_calls[i] != nullptr; i++) {
    if (strstr(content, banned_calls[i]) != nullptr) {
      snprintf(result->reason, sizeof(result->reason),
               "Banned call detected: %s", banned_calls[i]);
      result->passed = false;
      return false;
    }
  }

  // Check for banned includes
  for (int i = 0; banned_includes[i] != nullptr; i++) {
    if (strstr(content, banned_includes[i]) != nullptr) {
      snprintf(result->reason, sizeof(result->reason),
               "Banned include: %s", banned_includes[i]);
      result->passed = false;
      return false;
    }
  }

  // Check for inline assembly (concern, not fatal unless Grade < 4)
  if (strstr(content, "__asm__") != nullptr || strstr(content, "asm(") != nullptr ||
      strstr(content, "__asm(") != nullptr) {
    if (result->concern_count < 4) {
      strncpy(result->concerns[result->concern_count++],
              "Contains inline assembly", 255);
    }
  }

  // Check for suspicious preprocessor directives
  if (strstr(content, "#pragma comment(linker") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "Linker pragma detected (Windows-style injection)");
    result->passed = false;
    return false;
  }

  return true;
}

// ============================================================================
// Content Validation — Assembly
// ============================================================================

bool JvmResourceLoader::validate_asm_source(const char* content, size_t len,
                                             ValidationResult* result) {
  result->passed = true;
  result->concern_count = 0;

  // Must not contain ELF interpreter injection
  if (strstr(content, ".section .interp") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "ELF .interp section detected (interpreter injection)");
    result->passed = false;
    return false;
  }

  // Must not contain syscall to exec
  if (strstr(content, "sys_execve") != nullptr || strstr(content, "__NR_execve") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "execve syscall reference detected");
    result->passed = false;
    return false;
  }

  // Should target x86_64 (check for non-x86 directives as concern)
  if (strstr(content, ".arch armv") != nullptr ||
      strstr(content, ".arch aarch64") != nullptr) {
    if (result->concern_count < 4) {
      strncpy(result->concerns[result->concern_count++],
              "Non-x86_64 architecture directives present", 255);
    }
  }

  return true;
}

// ============================================================================
// Content Validation — C++ Header
// ============================================================================

bool JvmResourceLoader::validate_hpp_header(const char* content, size_t len,
                                             ValidationResult* result) {
  result->passed = true;
  result->concern_count = 0;

  // Should have include guard or #pragma once
  bool has_guard = (strstr(content, "#ifndef") != nullptr && strstr(content, "#define") != nullptr);
  bool has_pragma_once = (strstr(content, "#pragma once") != nullptr);

  if (!has_guard && !has_pragma_once) {
    if (result->concern_count < 4) {
      strncpy(result->concerns[result->concern_count++],
              "No include guard or #pragma once", 255);
    }
  }

  // Should not define macros that shadow JVM internals
  static const char* jvm_macros[] = {
    "#define ASSERT",
    "#define guarantee",
    "#define vm_exit",
    "#define os_malloc",
    nullptr
  };

  for (int i = 0; jvm_macros[i] != nullptr; i++) {
    if (strstr(content, jvm_macros[i]) != nullptr) {
      snprintf(result->reason, sizeof(result->reason),
               "Macro shadows JVM internal: %s", jvm_macros[i]);
      result->passed = false;
      return false;
    }
  }

  return true;
}

// ============================================================================
// Content Validation — JSON
// ============================================================================

bool JvmResourceLoader::validate_json(const char* content, size_t len,
                                       ValidationResult* result) {
  result->passed = true;
  result->concern_count = 0;

  if (len == 0) {
    snprintf(result->reason, sizeof(result->reason), "Empty file");
    result->passed = false;
    return false;
  }

  // Basic structural validation
  // Must start with { or [
  const char* p = content;
  while (p < content + len && isspace((unsigned char)*p)) p++;
  if (*p != '{' && *p != '[') {
    snprintf(result->reason, sizeof(result->reason),
             "JSON must start with { or [ (found '%c')", *p);
    result->passed = false;
    return false;
  }

  // Check nesting depth (max 32)
  int max_depth = 0;
  int current_depth = 0;
  for (size_t i = 0; i < len; i++) {
    if (content[i] == '{' || content[i] == '[') {
      current_depth++;
      if (current_depth > max_depth) max_depth = current_depth;
    } else if (content[i] == '}' || content[i] == ']') {
      current_depth--;
    }
  }

  if (max_depth > 32) {
    snprintf(result->reason, sizeof(result->reason),
             "JSON nesting depth %d exceeds maximum (32)", max_depth);
    result->passed = false;
    return false;
  }

  // Check for embedded executable content
  if (strstr(content, "<script") != nullptr || strstr(content, "javascript:") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "JSON contains embedded script content");
    result->passed = false;
    return false;
  }

  return true;
}

// ============================================================================
// Content Validation — XML
// ============================================================================

bool JvmResourceLoader::validate_xml(const char* content, size_t len,
                                      ValidationResult* result) {
  result->passed = true;
  result->concern_count = 0;

  if (len == 0) {
    snprintf(result->reason, sizeof(result->reason), "Empty file");
    result->passed = false;
    return false;
  }

  // Security: no DTD, no external entities, no SYSTEM
  if (strstr(content, "<!DOCTYPE") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "DOCTYPE declaration not allowed (XXE prevention)");
    result->passed = false;
    return false;
  }

  if (strstr(content, "<!ENTITY") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "ENTITY declaration not allowed (XXE prevention)");
    result->passed = false;
    return false;
  }

  if (strstr(content, "SYSTEM") != nullptr) {
    snprintf(result->reason, sizeof(result->reason),
             "SYSTEM reference not allowed (external entity prevention)");
    result->passed = false;
    return false;
  }

  // Check nesting depth (max 64 for XML)
  int max_depth = 0;
  int current_depth = 0;
  bool in_tag = false;
  bool is_close = false;

  for (size_t i = 0; i < len; i++) {
    if (content[i] == '<') {
      in_tag = true;
      is_close = (i + 1 < len && content[i + 1] == '/');
      if (is_close) {
        current_depth--;
      }
    } else if (content[i] == '>') {
      if (in_tag && !is_close && (i > 0 && content[i - 1] != '/')) {
        current_depth++;
        if (current_depth > max_depth) max_depth = current_depth;
      }
      in_tag = false;
    }
  }

  if (max_depth > 64) {
    snprintf(result->reason, sizeof(result->reason),
             "XML nesting depth %d exceeds maximum (64)", max_depth);
    result->passed = false;
    return false;
  }

  return true;
}

// ============================================================================
// Secure File Read
// ============================================================================

bool JvmResourceLoader::read_file_secure(const char* path, char** out_content, size_t* out_len) {
  struct stat st;
  if (stat(path, &st) != 0) return false;

  // Must be regular file
  if (!S_ISREG(st.st_mode)) return false;

  // Must not be world-writable
  if (st.st_mode & S_IWOTH) {
    log_warning(os)("JvmResourceLoader: refusing world-writable file: %s", path);
    return false;
  }

  size_t file_size = (size_t)st.st_size;

  // Type-based size limit
  ResourceType type = detect_type(path);
  size_t max_size = MAX_JSON_SIZE;  // Default
  switch (type) {
    case RESOURCE_C:   max_size = MAX_C_SIZE; break;
    case RESOURCE_ASM: max_size = MAX_ASM_SIZE; break;
    case RESOURCE_HPP: max_size = MAX_HPP_SIZE; break;
    case RESOURCE_JSON: max_size = MAX_JSON_SIZE; break;
    case RESOURCE_XML:  max_size = MAX_XML_SIZE; break;
    default: break;
  }

  if (file_size > max_size) {
    log_warning(os)("JvmResourceLoader: file too large (%zu > %zu): %s",
                    file_size, max_size, path);
    return false;
  }

  // Open with O_NOFOLLOW (no symlink traversal)
  int fd = open(path, O_RDONLY | O_NOFOLLOW);
  if (fd < 0) return false;

  char* content = (char*)os::malloc(file_size + 1, mtInternal);
  if (content == nullptr) {
    close(fd);
    return false;
  }

  ssize_t bytes = read(fd, content, file_size);
  close(fd);

  if (bytes != (ssize_t)file_size) {
    os::free(content);
    return false;
  }

  content[file_size] = '\0';
  *out_content = content;
  *out_len = file_size;
  return true;
}

// ============================================================================
// SHA-256 (simplified placeholder — production would use system crypto)
// ============================================================================

void JvmResourceLoader::compute_sha256(const char* content, size_t len, char* out_hex) {
  // Simple hash placeholder — in production, use OpenSSL or system SHA-256
  // For now, compute a fingerprint from content sampling
  unsigned long hash = 5381;
  for (size_t i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + (unsigned char)content[i];
  }
  snprintf(out_hex, 65, "%016lx%016lx%016lx%016lx",
           hash, hash ^ 0xDEADBEEF, hash ^ 0xCAFEBABE, hash ^ 0x12345678);
}

// ============================================================================
// Record Creation
// ============================================================================

LoadedResource* JvmResourceLoader::create_record(const char* path, ResourceType type,
                                                  const char* content, size_t len,
                                                  LoaderGrade grade, const char* caller) {
  LoadedResource* rec = (LoadedResource*)os::malloc(sizeof(LoadedResource), mtInternal);
  if (rec == nullptr) return nullptr;

  memset(rec, 0, sizeof(LoadedResource));
  rec->resource_id = _next_resource_id++;
  rec->type = type;
  strncpy(rec->path, path, sizeof(rec->path) - 1);

  // Extract filename
  const char* slash = strrchr(path, '/');
  strncpy(rec->filename, slash ? slash + 1 : path, sizeof(rec->filename) - 1);

  rec->file_size = len;
  compute_sha256(content, len, rec->sha256);
  rec->loaded_by_grade = grade;
  strncpy(rec->loaded_by, caller ? caller : "unknown", sizeof(rec->loaded_by) - 1);
  rec->loaded_at_ms = os::elapsed_counter() / (os::elapsed_frequency() / 1000);
  rec->status = CONTENT_PENDING;

  // Copy content into managed memory
  rec->content = (char*)os::malloc(len + 1, mtInternal);
  if (rec->content != nullptr) {
    memcpy(rec->content, content, len);
    rec->content[len] = '\0';
    rec->content_len = len;
  }

  return rec;
}

// ============================================================================
// LOAD RESOURCE — Main Entry Point
// ============================================================================

int JvmResourceLoader::load_resource(const char* path, LoaderGrade grade, const char* caller) {
  ResourceType type = detect_type(path);
  if (type == RESOURCE_UNKNOWN) {
    log_warning(os)("JvmResourceLoader: unknown file type: %s", path);
    return -1;
  }
  return load_resource_typed(path, type, grade, caller);
}

int JvmResourceLoader::load_resource_typed(const char* path, ResourceType type,
                                            LoaderGrade grade, const char* caller) {
  if (!_enabled) return -1;

  log_info(os)("JvmResourceLoader: load request [%s] type=%s grade=%s caller=%s",
               path, type_name(type), grade_name(grade), caller ? caller : "?");

  // Step 1: Permission check
  if (!check_permission(type, grade)) {
    log_warning(os)("JvmResourceLoader: PERMISSION DENIED [%s needs grade %d, have %d]",
                    type_name(type), (type <= RESOURCE_XML ? 1 : type <= RESOURCE_HPP ? 2 : 3),
                    (int)grade);
    _total_rejected++;
    return -1;
  }

  // Step 2: Read file securely
  char* content = nullptr;
  size_t content_len = 0;
  if (!read_file_secure(path, &content, &content_len)) {
    log_warning(os)("JvmResourceLoader: failed to read file: %s", path);
    _total_rejected++;
    return -1;
  }

  // Step 3: Validate content (unless Kernel grade bypasses)
  ValidationResult validation;
  memset(&validation, 0, sizeof(validation));
  bool valid = true;

  if (grade < LOADER_GRADE_KERNEL) {
    switch (type) {
      case RESOURCE_C:    valid = validate_c_source(content, content_len, &validation); break;
      case RESOURCE_ASM:  valid = validate_asm_source(content, content_len, &validation); break;
      case RESOURCE_HPP:  valid = validate_hpp_header(content, content_len, &validation); break;
      case RESOURCE_JSON: valid = validate_json(content, content_len, &validation); break;
      case RESOURCE_XML:  valid = validate_xml(content, content_len, &validation); break;
      default: break;
    }
  }

  // Step 4: Create inventory record
  LoadedResource* rec = create_record(path, type, content, content_len, grade, caller);
  os::free(content);  // create_record copies it

  if (rec == nullptr) {
    _total_rejected++;
    return -1;
  }

  // Step 5: Apply validation result
  if (!valid) {
    rec->status = CONTENT_QUARANTINED;
    log_warning(os)("JvmResourceLoader: QUARANTINED [%s] reason: %s",
                    path, validation.reason);
    _total_rejected++;
  } else {
    rec->status = CONTENT_ACTIVE;
    _total_loaded++;

    if (validation.concern_count > 0) {
      log_info(os)("JvmResourceLoader: loaded with %d concern(s): %s",
                   validation.concern_count, path);
      for (int i = 0; i < validation.concern_count; i++) {
        log_info(os)("  concern: %s", validation.concerns[i]);
      }
    }
  }

  // Step 6: Register in inventory
  lock();
  rec->next = _inventory_head;
  _inventory_head = rec;
  unlock();

  log_info(os)("JvmResourceLoader: %s [id=%d, type=%s, size=%zu, status=%s]",
               valid ? "LOADED" : "QUARANTINED",
               rec->resource_id, type_name(type), rec->file_size,
               status_name(rec->status));

  return valid ? rec->resource_id : -1;
}

// ============================================================================
// Content Access
// ============================================================================

const char* JvmResourceLoader::get_content(int resource_id) {
  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    if (r->resource_id == resource_id && r->status == CONTENT_ACTIVE) {
      unlock();
      return r->content;
    }
    r = r->next;
  }
  unlock();
  return nullptr;
}

size_t JvmResourceLoader::get_content_length(int resource_id) {
  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    if (r->resource_id == resource_id) {
      size_t len = r->content_len;
      unlock();
      return len;
    }
    r = r->next;
  }
  unlock();
  return 0;
}

ContentStatus JvmResourceLoader::get_status(int resource_id) {
  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    if (r->resource_id == resource_id) {
      ContentStatus s = r->status;
      unlock();
      return s;
    }
    r = r->next;
  }
  unlock();
  return CONTENT_REJECTED;
}

// ============================================================================
// Resource Management
// ============================================================================

bool JvmResourceLoader::release_resource(int resource_id) {
  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    if (r->resource_id == resource_id) {
      if (r->content != nullptr) {
        os::free(r->content);
        r->content = nullptr;
        r->content_len = 0;
      }
      r->status = CONTENT_REJECTED;
      unlock();
      return true;
    }
    r = r->next;
  }
  unlock();
  return false;
}

bool JvmResourceLoader::quarantine_resource(int resource_id) {
  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    if (r->resource_id == resource_id) {
      r->status = CONTENT_QUARANTINED;
      unlock();
      return true;
    }
    r = r->next;
  }
  unlock();
  return false;
}

// ============================================================================
// Inventory Display
// ============================================================================

void JvmResourceLoader::print_inventory(outputStream* st) {
  st->print_cr("╔══════════════════════════════════════════════════════════════════╗");
  st->print_cr("║  RESOURCE INVENTORY                                              ║");
  st->print_cr("╚══════════════════════════════════════════════════════════════════╝");
  st->print_cr("  Total loaded: %d, Rejected: %d", _total_loaded, _total_rejected);
  st->cr();
  st->print_cr("  ID  | Type       | Size     | Status      | Grade   | File");
  st->print_cr("  ----+------------+----------+-------------+---------+------");

  lock();
  LoadedResource* r = _inventory_head;
  while (r != nullptr) {
    st->print_cr("  %3d | %-10s | %7zu B| %-11s | Grade %d | %s",
                 r->resource_id, type_name(r->type), r->file_size,
                 status_name(r->status), (int)r->loaded_by_grade, r->filename);
    r = r->next;
  }
  unlock();
}

void JvmResourceLoader::print_status(outputStream* st) {
  st->print_cr("JVM Resource Loader Status:");
  st->print_cr("  Enabled:         %s", _enabled ? "yes" : "no");
  st->print_cr("  Total loaded:    %d", _total_loaded);
  st->print_cr("  Total rejected:  %d", _total_rejected);
  st->print_cr("  Inventory size:  %d records", _total_loaded + _total_rejected);
}

void JvmResourceLoader::print_permissions(outputStream* st) {
  st->print_cr("Resource Loading Permissions:");
  st->print_cr("  Grade 1 (Application): JSON, XML");
  st->print_cr("  Grade 2 (Trusted):     JSON, XML, HPP");
  st->print_cr("  Grade 3 (System):      JSON, XML, HPP, C, S");
  st->print_cr("  Grade 4 (Kernel):      All types, bypass validation");
}
