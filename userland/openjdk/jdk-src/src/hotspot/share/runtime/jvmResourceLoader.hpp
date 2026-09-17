/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * jvmResourceLoader.hpp - Secure Resource Loader for Native File Types
 *
 * Provides careful, permission-gated loading of non-Java resources into
 * the JVM environment. Each file type has its own appropriation pathway
 * with validation, permission checks, and content grading.
 *
 * SUPPORTED FILE TYPES:
 *   .c    — C source files (for JNI compilation, plugin sources)
 *   .S/.s — Assembly source files (architecture-specific routines)
 *   .hpp  — C++ header files (native interface declarations)
 *   .json — JSON configuration and data files
 *   .xml  — XML configuration, schema, and data files
 *
 * APPROPRIATION MODEL:
 *   Loading a resource is not automatic. The loader must:
 *   1. IDENTIFY the file (path, type, size, checksum)
 *   2. VALIDATE the content (type-specific safety checks)
 *   3. PERMISSION CHECK (does the caller have adequate grade?)
 *   4. APPROPRIATE (read into managed JVM memory with tracking)
 *   5. REGISTER (file becomes part of the JVM's resource inventory)
 *
 * PERMISSION GRADES (who can load what):
 *   Grade 1 (Application): JSON, XML only (data files)
 *   Grade 2 (Trusted):     JSON, XML, HPP (headers for inspection)
 *   Grade 3 (System):      JSON, XML, HPP, C, S (full native loading)
 *   Grade 4 (Kernel):      All types + unrestricted size + bypass validation
 *
 * CONTENT VALIDATION:
 *   .c   — No #include of banned headers, no inline asm (unless Grade 4)
 *          No exec/system/fork calls, no dlopen references
 *   .S   — Must target declared architecture (x86_64)
 *          No .section .interp (no ELF interpreter injection)
 *   .hpp — No #pragma once without matching guard
 *          No macro definitions that shadow JVM internals
 *   .json— Valid JSON syntax, max nesting depth 32, max 10MB
 *          No embedded scripts or executable content
 *   .xml — No DTD, no external entities, no SYSTEM references
 *          Max 10MB, max nesting depth 64
 *
 * INVENTORY:
 *   All loaded resources are tracked in an inventory:
 *   - Path, type, size, SHA-256 hash
 *   - Who loaded it (permission grade, caller identity)
 *   - When loaded (timestamp since JVM start)
 *   - Content status (validated, quarantined, active)
 */

#ifndef SHARE_RUNTIME_JVMRESOURCELOADER_HPP
#define SHARE_RUNTIME_JVMRESOURCELOADER_HPP

#include "memory/allocation.hpp"
#include "utilities/ostream.hpp"

// ============================================================================
// File Types
// ============================================================================

enum ResourceType {
  RESOURCE_C      = 0,  // .c source file
  RESOURCE_ASM    = 1,  // .S/.s assembly source
  RESOURCE_HPP    = 2,  // .hpp C++ header
  RESOURCE_JSON   = 3,  // .json data/config
  RESOURCE_XML    = 4,  // .xml data/config
  RESOURCE_UNKNOWN = 5
};

// ============================================================================
// Permission Grades
// ============================================================================

enum LoaderGrade {
  LOADER_GRADE_APPLICATION = 1,  // JSON, XML only
  LOADER_GRADE_TRUSTED     = 2,  // +HPP
  LOADER_GRADE_SYSTEM      = 3,  // +C, +S
  LOADER_GRADE_KERNEL      = 4   // All, unrestricted
};

// ============================================================================
// Content Status
// ============================================================================

enum ContentStatus {
  CONTENT_PENDING     = 0,  // Awaiting validation
  CONTENT_VALIDATED   = 1,  // Passed all checks
  CONTENT_QUARANTINED = 2,  // Failed validation, held for review
  CONTENT_ACTIVE      = 3,  // Validated and in use
  CONTENT_REJECTED    = 4   // Permanently refused
};

// ============================================================================
// Loaded Resource Record
// ============================================================================

struct LoadedResource {
  int             resource_id;
  ResourceType    type;
  char            path[512];
  char            filename[256];
  size_t          file_size;
  char            sha256[65];        // Hex-encoded SHA-256
  LoaderGrade     loaded_by_grade;
  char            loaded_by[256];    // Caller identity
  uint64_t        loaded_at_ms;
  ContentStatus   status;
  char*           content;           // The loaded content (managed memory)
  size_t          content_len;
  LoadedResource* next;
};

// ============================================================================
// Validation Result
// ============================================================================

struct ValidationResult {
  bool    passed;
  char    reason[512];       // Why it failed (if it did)
  int     concern_count;     // Number of non-fatal concerns
  char    concerns[4][256];  // Up to 4 noted concerns
};

// ============================================================================
// Resource Loader
// ============================================================================

class JvmResourceLoader : public AllStatic {
private:
  static bool             _enabled;
  static LoadedResource*  _inventory_head;
  static int              _next_resource_id;
  static int              _total_loaded;
  static int              _total_rejected;
  static volatile int     _lock;

  // Locking
  static void lock();
  static void unlock();

  // Type detection
  static ResourceType detect_type(const char* path);
  static const char* type_name(ResourceType type);
  static const char* type_extension(ResourceType type);

  // Permission check
  static bool check_permission(ResourceType type, LoaderGrade grade);

  // Content validation (per-type)
  static bool validate_c_source(const char* content, size_t len, ValidationResult* result);
  static bool validate_asm_source(const char* content, size_t len, ValidationResult* result);
  static bool validate_hpp_header(const char* content, size_t len, ValidationResult* result);
  static bool validate_json(const char* content, size_t len, ValidationResult* result);
  static bool validate_xml(const char* content, size_t len, ValidationResult* result);

  // File I/O
  static bool read_file_secure(const char* path, char** out_content, size_t* out_len);
  static void compute_sha256(const char* content, size_t len, char* out_hex);

  // Inventory management
  static LoadedResource* create_record(const char* path, ResourceType type,
                                       const char* content, size_t len,
                                       LoaderGrade grade, const char* caller);

public:
  // =========================================================================
  // Initialization
  // =========================================================================
  static void initialize();

  // =========================================================================
  // LOAD RESOURCE — Main entry point
  //
  // Performs full appropriation pipeline:
  //   identify → validate → permission → appropriate → register
  //
  // Returns resource_id on success, -1 on failure.
  // Content is accessible via get_content(resource_id).
  // =========================================================================
  static int load_resource(const char* path, LoaderGrade grade, const char* caller);

  // =========================================================================
  // Load with explicit type (skip detection)
  // =========================================================================
  static int load_resource_typed(const char* path, ResourceType type,
                                 LoaderGrade grade, const char* caller);

  // =========================================================================
  // Content Access
  // =========================================================================
  static const char* get_content(int resource_id);
  static size_t      get_content_length(int resource_id);
  static ContentStatus get_status(int resource_id);

  // =========================================================================
  // Inventory Query
  // =========================================================================
  static void print_inventory(outputStream* st);
  static void print_resource(int resource_id, outputStream* st);
  static int  total_loaded()   { return _total_loaded; }
  static int  total_rejected() { return _total_rejected; }

  // =========================================================================
  // Resource Management
  // =========================================================================
  static bool release_resource(int resource_id);        // Free content memory
  static bool quarantine_resource(int resource_id);     // Mark as quarantined
  static bool revalidate_resource(int resource_id);     // Re-run validation

  // =========================================================================
  // Diagnostics
  // =========================================================================
  static void print_status(outputStream* st);
  static void print_permissions(outputStream* st);
  static const char* grade_name(LoaderGrade grade);
  static const char* status_name(ContentStatus status);
};

#endif // SHARE_RUNTIME_JVMRESOURCELOADER_HPP
