/*
 * Copyright (C) 2026 MEARVK LLC. All rights reserved.
 *
 * xmlClassReader.hpp — XML Class File Reader for the Secure JVM
 * Galactic Cherry Marvell Edition 98
 *
 * Reads .xclass (XML class files) and converts them into the internal
 * ClassFileStream format that the standard ClassFileParser expects.
 * This allows the JVM to load classes from a richer, human-readable
 * XML format that carries provenance, design intent, security metadata,
 * optimization hints, and inline test contracts — none of which the
 * binary .class format can express.
 *
 * Detection: The reader is invoked when the first bytes of a class file
 * stream begin with "<?xml" or "<?xclass" instead of 0xCAFEBABE.
 *
 * Pipeline:
 *   .xclass XML → xmlClassReader → ClassFileStream (binary) → ClassFileParser
 *
 * The reader also extracts and registers rich metadata with:
 *   - ClassLoadGuard (security.classload-grade)
 *   - Integrity Guardian (security.trust-grade)
 *   - Observer Circuit (provenance, history)
 *   - Memory Proxy (security.resource-budget)
 *   - System Codex (design.intent, design.pattern)
 */

#ifndef SHARE_CLASSFILE_XMLCLASSREADER_HPP
#define SHARE_CLASSFILE_XMLCLASSREADER_HPP

#include "memory/allocation.hpp"
#include "utilities/exceptions.hpp"
#include "utilities/growableArray.hpp"

class ClassFileStream;
class Symbol;
class ClassLoaderData;

// ═══════════════════════════════════════════════════════════════════════════════
// XML Class File Magic Detection
// ═══════════════════════════════════════════════════════════════════════════════

// Check if a buffer starts with XML rather than 0xCAFEBABE
// Returns true if the stream is an XML class file
inline bool is_xml_class_file(const unsigned char* buffer, int length) {
  if (length < 5) return false;
  // "<?xml" or "<?xcl" (for <?xclass)
  return (buffer[0] == '<' && buffer[1] == '?') ||
         // UTF-8 BOM + "<?xml"
         (length >= 8 && buffer[0] == 0xEF && buffer[1] == 0xBB &&
          buffer[2] == 0xBF && buffer[3] == '<' && buffer[4] == '?');
}

// ═══════════════════════════════════════════════════════════════════════════════
// Rich Metadata Structures (extracted from XML, not in binary .class)
// ═══════════════════════════════════════════════════════════════════════════════

// Provenance information
struct XClassProvenance : public CHeapObj<mtClass> {
  const char* author;
  const char* organization;
  const char* created;
  const char* modified;
  const char* license;
  const char* source_file;
  const char* repository;
  const char* commit;

  XClassProvenance() : author(nullptr), organization(nullptr), created(nullptr),
                       modified(nullptr), license(nullptr), source_file(nullptr),
                       repository(nullptr), commit(nullptr) {}
};

// Design intent metadata
struct XClassDesign : public CHeapObj<mtClass> {
  const char* intent;
  const char* pattern;
  const char* thread_safety;    // "thread-safe", "not-thread-safe", "immutable"
  const char* nullability;      // "nullable", "no-null-returns", "strict-nonnull"

  // Contract arrays
  GrowableArray<const char*>* preconditions;
  GrowableArray<const char*>* postconditions;
  GrowableArray<const char*>* invariants;

  XClassDesign() : intent(nullptr), pattern(nullptr), thread_safety(nullptr),
                   nullability(nullptr), preconditions(nullptr),
                   postconditions(nullptr), invariants(nullptr) {}
};

// Security metadata
struct XClassSecurity : public CHeapObj<mtClass> {
  int trust_grade;          // 0-5 (maps to Extended Permission Classes)
  int classload_grade;      // 0-7 (maps to ClassLoadGuard grades)
  int64_t max_memory;       // bytes, -1 for unlimited
  int max_threads;          // -1 for unlimited
  int max_file_descriptors; // -1 for unlimited

  GrowableArray<const char*>* permissions;

  XClassSecurity() : trust_grade(-1), classload_grade(-1), max_memory(-1),
                     max_threads(-1), max_file_descriptors(-1),
                     permissions(nullptr) {}
};

// Dependency declaration
struct XClassDependency : public CHeapObj<mtClass> {
  const char* name;
  const char* version;
  const char* type;     // "module", "package", "library"
  bool required;

  XClassDependency() : name(nullptr), version(nullptr), type(nullptr),
                       required(true) {}
};

// Optimization hints
struct XClassHint : public CHeapObj<mtClass> {
  GrowableArray<const char*>* hot_methods;
  GrowableArray<const char*>* inline_candidates;

  XClassHint() : hot_methods(nullptr), inline_candidates(nullptr) {}
};

// Method-level contract
struct XMethodContract : public CHeapObj<mtClass> {
  const char* method_name;
  GrowableArray<const char*>* preconditions;
  GrowableArray<const char*>* postconditions;
  GrowableArray<const char*>* throws_decl;

  XMethodContract() : method_name(nullptr), preconditions(nullptr),
                      postconditions(nullptr), throws_decl(nullptr) {}
};

// Test specification
struct XClassTest : public CHeapObj<mtClass> {
  const char* method_name;
  const char* test_name;
  const char* given;
  GrowableArray<const char*>* expectations;
  const char* expect_throws;

  XClassTest() : method_name(nullptr), test_name(nullptr), given(nullptr),
                 expectations(nullptr), expect_throws(nullptr) {}
};

// History revision
struct XClassRevision : public CHeapObj<mtClass> {
  const char* version;
  const char* date;
  const char* author;
  const char* description;

  XClassRevision() : version(nullptr), date(nullptr), author(nullptr),
                     description(nullptr) {}
};

// Complete rich metadata container
struct XClassMetadata : public CHeapObj<mtClass> {
  XClassProvenance*                provenance;
  XClassDesign*                    design;
  XClassSecurity*                  security;
  XClassHint*                      hints;
  GrowableArray<XClassDependency*>* dependencies;
  GrowableArray<XMethodContract*>*  method_contracts;
  GrowableArray<XClassTest*>*       tests;
  GrowableArray<XClassRevision*>*   history;
  const char*                      documentation_summary;
  const char*                      documentation_details;

  XClassMetadata() : provenance(nullptr), design(nullptr), security(nullptr),
                     hints(nullptr), dependencies(nullptr),
                     method_contracts(nullptr), tests(nullptr),
                     history(nullptr), documentation_summary(nullptr),
                     documentation_details(nullptr) {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// XMLClassReader — Main Reader Class
// ═══════════════════════════════════════════════════════════════════════════════

class XMLClassReader : public StackObj {
 private:
  const unsigned char* _buffer;
  int _length;
  const char* _source;

  // Parsed state
  XClassMetadata* _metadata;

  // Internal XML parser state
  const char* _pos;
  const char* _end;

  // Assembled binary class file
  GrowableArray<unsigned char>* _class_bytes;

  // Parsing methods
  bool parse_xml();
  bool parse_identity();
  bool parse_provenance();
  bool parse_documentation();
  bool parse_design();
  bool parse_security();
  bool parse_dependencies();
  bool parse_constant_pool();
  bool parse_fields();
  bool parse_methods();
  bool parse_hints();
  bool parse_tests();
  bool parse_history();
  bool parse_annotations();
  bool parse_inner_classes();

  // XML utility methods
  bool skip_whitespace();
  bool skip_comment();
  bool expect_tag(const char* tag_name);
  bool read_tag_name(char* buf, int buflen);
  bool read_attribute(const char* attr_name, char* buf, int buflen);
  bool read_element_text(char* buf, int buflen);
  bool read_cdata(unsigned char* buf, int buflen, int* out_len);
  bool skip_to_end_tag(const char* tag_name);
  int  base64_decode(const char* input, int input_len, unsigned char* output, int output_max);

  // Binary class file assembly
  void emit_u1(unsigned char val);
  void emit_u2(unsigned short val);
  void emit_u4(unsigned int val);
  void emit_bytes(const unsigned char* data, int len);

  // Constant pool assembly
  void assemble_constant_pool();
  void assemble_fields();
  void assemble_methods();
  void assemble_attributes();

  // Metadata registration (feeds other Secure JVM modules)
  void register_metadata_with_classload_guard();
  void register_metadata_with_integrity_guardian();
  void register_metadata_with_observer_circuit();
  void register_metadata_with_memory_proxy();
  void register_metadata_with_codex();

 public:
  XMLClassReader(const unsigned char* buffer, int length, const char* source);
  ~XMLClassReader();

  // Main entry point: parse XML and produce a binary ClassFileStream
  // Returns nullptr on failure (sets TRAPS)
  ClassFileStream* read_xml_class(TRAPS);

  // Access to rich metadata (available after successful read)
  XClassMetadata* metadata() const { return _metadata; }

  // Class name extracted from XML (for early identification)
  const char* parsed_class_name() const;

  // Validation
  bool validate_signature() const;
  bool validate_bytecode_integrity() const;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Global Registry — Stores rich metadata keyed by class name
// ═══════════════════════════════════════════════════════════════════════════════

class XMLClassMetadataRegistry : public AllStatic {
 private:
  // Hash table: class name → XClassMetadata*
  static const int TABLE_SIZE = 1024;
  static XClassMetadata* _table[];
  static const char* _names[];
  static int _count;

 public:
  static void initialize();

  // Register metadata for a loaded class
  static void register_metadata(const char* class_name, XClassMetadata* metadata);

  // Query metadata for a loaded class (returns nullptr if class was loaded from binary)
  static XClassMetadata* lookup(const char* class_name);

  // Query provenance
  static XClassProvenance* get_provenance(const char* class_name);

  // Query design intent
  static XClassDesign* get_design(const char* class_name);

  // Query security requirements
  static XClassSecurity* get_security(const char* class_name);

  // Query optimization hints
  static XClassHint* get_hints(const char* class_name);

  // Dump all registered metadata (for observer circuit)
  static void dump_all(outputStream* st);

  // Statistics
  static int count() { return _count; }
};

#endif // SHARE_CLASSFILE_XMLCLASSREADER_HPP
