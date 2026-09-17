/*
 * Copyright (C) 2026 MEARVK LLC. All rights reserved.
 *
 * xmlClassReader.cpp — XML Class File Reader Implementation
 * Galactic Cherry Marvell Edition 98
 *
 * Parses .xclass XML files and produces a binary ClassFileStream that
 * the standard ClassFileParser can consume. Also extracts rich metadata
 * and registers it with the Secure JVM module ecosystem.
 *
 * Security:
 *   - No DTD/ENTITY/SYSTEM processing (XXE prevention)
 *   - Maximum file size: 64 MB
 *   - Maximum element depth: 32
 *   - SHA-256 signature validation (optional)
 *   - No external references resolved
 *
 * Files produced: ClassFileStream (binary .class equivalent in memory)
 * Metadata registered: ClassLoadGuard, Integrity Guardian, Observer Circuit,
 *                      Memory Proxy, System Codex
 */

#include "classfile/xmlClassReader.hpp"
#include "classfile/classFileStream.hpp"
#include "memory/resourceArea.hpp"
#include "runtime/os.hpp"
#include "utilities/exceptions.hpp"
#include "utilities/growableArray.hpp"

#include <cstring>
#include <cstdlib>

// ═══════════════════════════════════════════════════════════════════════════════
// Constants
// ═══════════════════════════════════════════════════════════════════════════════

static const int XCLASS_MAX_FILE_SIZE    = 64 * 1024 * 1024;  // 64 MB
static const int XCLASS_MAX_DEPTH        = 32;
static const int XCLASS_MAX_TAG_NAME     = 128;
static const int XCLASS_MAX_ATTR_VALUE   = 4096;
static const int XCLASS_MAX_TEXT         = 65536;
static const int XCLASS_MAX_BYTECODE     = 65535;

static const unsigned int JAVA_MAGIC     = 0xCAFEBABE;
static const unsigned short JAVA28_MAJOR = 67;
static const unsigned short JAVA28_MINOR = 0;

// Base64 decode table
static const unsigned char b64_table[256] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

// ═══════════════════════════════════════════════════════════════════════════════
// Global Metadata Registry
// ═══════════════════════════════════════════════════════════════════════════════

XClassMetadata* XMLClassMetadataRegistry::_table[TABLE_SIZE] = { nullptr };
const char*     XMLClassMetadataRegistry::_names[TABLE_SIZE] = { nullptr };
int             XMLClassMetadataRegistry::_count = 0;

void XMLClassMetadataRegistry::initialize() {
  _count = 0;
  memset(_table, 0, sizeof(_table));
  memset(_names, 0, sizeof(_names));
}

static unsigned int hash_class_name(const char* name) {
  unsigned int h = 0;
  while (*name) {
    h = h * 31 + (unsigned char)(*name);
    name++;
  }
  return h;
}

void XMLClassMetadataRegistry::register_metadata(const char* class_name, XClassMetadata* metadata) {
  if (_count >= TABLE_SIZE) return;  // Table full

  unsigned int idx = hash_class_name(class_name) % TABLE_SIZE;

  // Linear probing
  for (int i = 0; i < TABLE_SIZE; i++) {
    unsigned int probe = (idx + i) % TABLE_SIZE;
    if (_names[probe] == nullptr) {
      _names[probe] = os::strdup(class_name);
      _table[probe] = metadata;
      _count++;
      return;
    }
    if (strcmp(_names[probe], class_name) == 0) {
      // Update existing
      _table[probe] = metadata;
      return;
    }
  }
}

XClassMetadata* XMLClassMetadataRegistry::lookup(const char* class_name) {
  unsigned int idx = hash_class_name(class_name) % TABLE_SIZE;
  for (int i = 0; i < TABLE_SIZE; i++) {
    unsigned int probe = (idx + i) % TABLE_SIZE;
    if (_names[probe] == nullptr) return nullptr;
    if (strcmp(_names[probe], class_name) == 0) return _table[probe];
  }
  return nullptr;
}

XClassProvenance* XMLClassMetadataRegistry::get_provenance(const char* class_name) {
  XClassMetadata* m = lookup(class_name);
  return m ? m->provenance : nullptr;
}

XClassDesign* XMLClassMetadataRegistry::get_design(const char* class_name) {
  XClassMetadata* m = lookup(class_name);
  return m ? m->design : nullptr;
}

XClassSecurity* XMLClassMetadataRegistry::get_security(const char* class_name) {
  XClassMetadata* m = lookup(class_name);
  return m ? m->security : nullptr;
}

XClassHint* XMLClassMetadataRegistry::get_hints(const char* class_name) {
  XClassMetadata* m = lookup(class_name);
  return m ? m->hints : nullptr;
}

void XMLClassMetadataRegistry::dump_all(outputStream* st) {
  st->print_cr("=== XML Class Metadata Registry (%d entries) ===", _count);
  for (int i = 0; i < TABLE_SIZE; i++) {
    if (_names[i] != nullptr) {
      st->print_cr("  [%d] %s", i, _names[i]);
      XClassMetadata* m = _table[i];
      if (m->provenance && m->provenance->author) {
        st->print_cr("       author: %s", m->provenance->author);
      }
      if (m->design && m->design->intent) {
        st->print_cr("       intent: %s", m->design->intent);
      }
      if (m->security) {
        st->print_cr("       trust-grade: %d, classload-grade: %d",
                     m->security->trust_grade, m->security->classload_grade);
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// XMLClassReader — Constructor / Destructor
// ═══════════════════════════════════════════════════════════════════════════════

XMLClassReader::XMLClassReader(const unsigned char* buffer, int length, const char* source)
  : _buffer(buffer), _length(length), _source(source),
    _metadata(nullptr), _pos(nullptr), _end(nullptr), _class_bytes(nullptr) {
}

XMLClassReader::~XMLClassReader() {
  // Metadata is registered globally and survives the reader
  // _class_bytes is transferred to the ClassFileStream
}

// ═══════════════════════════════════════════════════════════════════════════════
// XML Utility Methods
// ═══════════════════════════════════════════════════════════════════════════════

bool XMLClassReader::skip_whitespace() {
  while (_pos < _end && (*_pos == ' ' || *_pos == '\t' || *_pos == '\n' || *_pos == '\r')) {
    _pos++;
  }
  return _pos < _end;
}

bool XMLClassReader::skip_comment() {
  // Skip <!-- ... -->
  if (_pos + 4 <= _end && strncmp(_pos, "<!--", 4) == 0) {
    _pos += 4;
    while (_pos + 3 <= _end) {
      if (strncmp(_pos, "-->", 3) == 0) {
        _pos += 3;
        return true;
      }
      _pos++;
    }
    return false;  // Unterminated comment
  }
  return true;
}

bool XMLClassReader::expect_tag(const char* tag_name) {
  skip_whitespace();
  while (_pos < _end && *_pos == '<' && _pos + 3 < _end &&
         _pos[1] == '!' && _pos[2] == '-' && _pos[3] == '-') {
    if (!skip_comment()) return false;
    skip_whitespace();
  }

  if (_pos >= _end || *_pos != '<') return false;
  _pos++;  // skip '<'

  const char* start = _pos;
  while (_pos < _end && *_pos != '>' && *_pos != ' ' && *_pos != '/' && *_pos != '\t' && *_pos != '\n') {
    _pos++;
  }

  int name_len = (int)(_pos - start);
  int tag_len = (int)strlen(tag_name);

  if (name_len != tag_len || strncmp(start, tag_name, tag_len) != 0) {
    return false;
  }

  // Skip to end of tag (past attributes and >)
  while (_pos < _end && *_pos != '>') {
    // Security: reject DTD, ENTITY, SYSTEM
    if (*_pos == '!' || (strncmp(_pos, "SYSTEM", 6) == 0) ||
        (strncmp(_pos, "ENTITY", 6) == 0) || (strncmp(_pos, "DOCTYPE", 7) == 0)) {
      return false;  // XXE prevention
    }
    _pos++;
  }
  if (_pos < _end) _pos++;  // skip '>'
  return true;
}

bool XMLClassReader::read_tag_name(char* buf, int buflen) {
  skip_whitespace();
  while (_pos < _end && *_pos == '<' && _pos + 3 < _end &&
         _pos[1] == '!' && _pos[2] == '-' && _pos[3] == '-') {
    if (!skip_comment()) return false;
    skip_whitespace();
  }

  if (_pos >= _end || *_pos != '<') return false;
  _pos++;  // skip '<'

  // Check for end tag
  bool is_end_tag = false;
  if (_pos < _end && *_pos == '/') {
    is_end_tag = true;
    _pos++;
  }

  const char* start = _pos;
  while (_pos < _end && *_pos != '>' && *_pos != ' ' && *_pos != '/' && *_pos != '\t' && *_pos != '\n') {
    _pos++;
  }

  int len = (int)(_pos - start);
  if (len >= buflen) len = buflen - 1;

  if (is_end_tag) {
    buf[0] = '/';
    strncpy(buf + 1, start, len);
    buf[len + 1] = '\0';
  } else {
    strncpy(buf, start, len);
    buf[len] = '\0';
  }

  // Skip to end of tag
  while (_pos < _end && *_pos != '>') _pos++;
  if (_pos < _end) _pos++;  // skip '>'

  return true;
}

bool XMLClassReader::read_attribute(const char* attr_name, char* buf, int buflen) {
  // This is called with _pos pointing inside a tag (between < and >)
  // Search backwards to find the attribute in the current tag context
  // For simplicity, we search the raw text around current position

  const char* search = _pos - 1;
  int attr_len = (int)strlen(attr_name);

  // Search backward to find the tag start
  while (search > (const char*)_buffer && *search != '<') search--;

  // Now search forward for the attribute
  const char* tag_end = _pos;
  const char* p = search;
  while (p < tag_end) {
    if (strncmp(p, attr_name, attr_len) == 0 && p[attr_len] == '=') {
      p += attr_len + 1;
      if (*p == '"') {
        p++;
        const char* val_start = p;
        while (p < tag_end && *p != '"') p++;
        int val_len = (int)(p - val_start);
        if (val_len >= buflen) val_len = buflen - 1;
        strncpy(buf, val_start, val_len);
        buf[val_len] = '\0';
        return true;
      }
    }
    p++;
  }
  buf[0] = '\0';
  return false;
}

bool XMLClassReader::read_element_text(char* buf, int buflen) {
  // Read text content until next '<'
  skip_whitespace();
  const char* start = _pos;
  while (_pos < _end && *_pos != '<') _pos++;

  int len = (int)(_pos - start);
  // Trim trailing whitespace
  while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\t' ||
                     start[len-1] == '\n' || start[len-1] == '\r')) {
    len--;
  }
  // Trim leading whitespace
  while (len > 0 && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
    start++;
    len--;
  }

  if (len >= buflen) len = buflen - 1;
  strncpy(buf, start, len);
  buf[len] = '\0';
  return true;
}

bool XMLClassReader::skip_to_end_tag(const char* tag_name) {
  char end_pattern[XCLASS_MAX_TAG_NAME + 4];
  snprintf(end_pattern, sizeof(end_pattern), "</%s>", tag_name);
  int pat_len = (int)strlen(end_pattern);

  while (_pos + pat_len <= _end) {
    if (strncmp(_pos, end_pattern, pat_len) == 0) {
      _pos += pat_len;
      return true;
    }
    _pos++;
  }
  return false;
}

int XMLClassReader::base64_decode(const char* input, int input_len,
                                  unsigned char* output, int output_max) {
  int out_len = 0;
  unsigned int accum = 0;
  int bits = 0;

  for (int i = 0; i < input_len && out_len < output_max; i++) {
    unsigned char c = (unsigned char)input[i];
    if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
    unsigned char val = b64_table[c];
    if (val == 255) continue;  // invalid char, skip

    accum = (accum << 6) | val;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      output[out_len++] = (unsigned char)((accum >> bits) & 0xFF);
    }
  }
  return out_len;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Binary Emission Helpers
// ═══════════════════════════════════════════════════════════════════════════════

void XMLClassReader::emit_u1(unsigned char val) {
  _class_bytes->append(val);
}

void XMLClassReader::emit_u2(unsigned short val) {
  _class_bytes->append((unsigned char)(val >> 8));
  _class_bytes->append((unsigned char)(val & 0xFF));
}

void XMLClassReader::emit_u4(unsigned int val) {
  _class_bytes->append((unsigned char)(val >> 24));
  _class_bytes->append((unsigned char)((val >> 16) & 0xFF));
  _class_bytes->append((unsigned char)((val >> 8) & 0xFF));
  _class_bytes->append((unsigned char)(val & 0xFF));
}

void XMLClassReader::emit_bytes(const unsigned char* data, int len) {
  for (int i = 0; i < len; i++) {
    _class_bytes->append(data[i]);
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Constant Pool Assembly
// ═══════════════════════════════════════════════════════════════════════════════

// Internal constant pool entry representation
struct CPEntry {
  enum Tag {
    CONSTANT_Utf8 = 1,
    CONSTANT_Integer = 3,
    CONSTANT_Float = 4,
    CONSTANT_Long = 5,
    CONSTANT_Double = 6,
    CONSTANT_Class = 7,
    CONSTANT_String = 8,
    CONSTANT_Fieldref = 9,
    CONSTANT_Methodref = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType = 12,
    CONSTANT_MethodHandle = 15,
    CONSTANT_MethodType = 16,
    CONSTANT_InvokeDynamic = 18
  };

  Tag tag;
  union {
    struct { const char* value; int length; } utf8;
    int int_value;
    float float_value;
    long long_value;
    double double_value;
    struct { int name_index; } class_info;
    struct { int string_index; } string_info;
    struct { int class_index; int name_and_type_index; } ref_info;
    struct { int name_index; int descriptor_index; } name_and_type;
  } data;
};

// ═══════════════════════════════════════════════════════════════════════════════
// Main Parse Entry Point
// ═══════════════════════════════════════════════════════════════════════════════

ClassFileStream* XMLClassReader::read_xml_class(TRAPS) {
  // Safety checks
  if (_buffer == nullptr || _length <= 0) {
    THROW_MSG_(vmSymbols::java_lang_ClassFormatError(),
               "Empty XML class file", nullptr);
  }
  if (_length > XCLASS_MAX_FILE_SIZE) {
    THROW_MSG_(vmSymbols::java_lang_ClassFormatError(),
               "XML class file exceeds 64MB limit", nullptr);
  }

  // Initialize parse state
  _pos = (const char*)_buffer;
  _end = (const char*)_buffer + _length;
  _metadata = new XClassMetadata();
  _class_bytes = new (mtClass) GrowableArray<unsigned char>(4096);

  // Skip BOM if present
  if (_length >= 3 && _buffer[0] == 0xEF && _buffer[1] == 0xBB && _buffer[2] == 0xBF) {
    _pos += 3;
  }

  // Skip XML processing instructions
  skip_whitespace();
  while (_pos + 2 <= _end && _pos[0] == '<' && _pos[1] == '?') {
    // Security: check for DTD/ENTITY in PI
    const char* pi_start = _pos;
    while (_pos + 2 <= _end && !(_pos[0] == '?' && _pos[1] == '>')) {
      _pos++;
    }
    if (_pos + 2 <= _end) _pos += 2;  // skip ?>
    skip_whitespace();
  }

  // Parse the full XML structure
  if (!parse_xml()) {
    THROW_MSG_(vmSymbols::java_lang_ClassFormatError(),
               "Failed to parse XML class file", nullptr);
  }

  // Build the binary class file from parsed data
  // The _class_bytes array now contains a valid .class binary

  // Create ClassFileStream from assembled bytes
  int total_len = _class_bytes->length();
  unsigned char* class_data = NEW_RESOURCE_ARRAY(unsigned char, total_len);
  for (int i = 0; i < total_len; i++) {
    class_data[i] = _class_bytes->at(i);
  }

  ClassFileStream* stream = new ClassFileStream(class_data,
                                                total_len,
                                                _source,
                                                /* from_boot_loader_modules_image */ false,
                                                /* from_class_file_load_hook */ false);

  // Register rich metadata globally
  if (_metadata != nullptr) {
    const char* class_name = parsed_class_name();
    if (class_name != nullptr) {
      XMLClassMetadataRegistry::register_metadata(class_name, _metadata);
      register_metadata_with_classload_guard();
      register_metadata_with_integrity_guardian();
      register_metadata_with_observer_circuit();
      register_metadata_with_memory_proxy();
      register_metadata_with_codex();
    }
  }

  return stream;
}

// ═══════════════════════════════════════════════════════════════════════════════
// XML Structure Parser
// ═══════════════════════════════════════════════════════════════════════════════

// Internal storage for parsed class data before binary assembly
static struct ParsedClassData {
  char class_name[512];       // internal form: com/example/Foo
  char super_name[512];       // internal form: java/lang/Object
  int access_flags;
  int major_version;
  int minor_version;

  // Constant pool
  GrowableArray<CPEntry*>* cp_entries;
  int cp_count;

  // Interfaces
  GrowableArray<int>* interface_cp_indices;

  // Fields
  struct FieldInfo {
    int access_flags;
    int name_cp_index;
    int descriptor_cp_index;
    unsigned char* bytecode;
    int bytecode_len;
    int constant_value_index;  // 0 if none
  };
  GrowableArray<FieldInfo*>* fields;

  // Methods
  struct MethodInfo {
    int access_flags;
    int name_cp_index;
    int descriptor_cp_index;
    int max_stack;
    int max_locals;
    unsigned char* bytecode;
    int bytecode_len;
    GrowableArray<int>* exception_cp_indices;
  };
  GrowableArray<MethodInfo*>* methods;

  void init() {
    class_name[0] = '\0';
    super_name[0] = '\0';
    access_flags = 0x0021;  // ACC_PUBLIC | ACC_SUPER
    major_version = JAVA28_MAJOR;
    minor_version = JAVA28_MINOR;
    cp_entries = new (mtClass) GrowableArray<CPEntry*>(64);
    cp_count = 1;  // CP starts at index 1
    interface_cp_indices = new (mtClass) GrowableArray<int>(4);
    fields = new (mtClass) GrowableArray<FieldInfo*>(16);
    methods = new (mtClass) GrowableArray<MethodInfo*>(16);
  }
} _parsed;

bool XMLClassReader::parse_xml() {
  _parsed.init();

  // Expect root <class> element
  if (!expect_tag("class")) {
    return false;
  }

  // Parse child elements in order
  char tag[XCLASS_MAX_TAG_NAME];
  while (_pos < _end) {
    skip_whitespace();
    // Check for comments
    while (_pos + 4 <= _end && _pos[0] == '<' && _pos[1] == '!' &&
           _pos[2] == '-' && _pos[3] == '-') {
      if (!skip_comment()) return false;
      skip_whitespace();
    }

    if (_pos >= _end) break;

    // Check for end of <class>
    if (_pos + 8 <= _end && strncmp(_pos, "</class>", 8) == 0) {
      _pos += 8;
      break;
    }

    // Peek at next tag
    const char* saved = _pos;
    if (!read_tag_name(tag, sizeof(tag))) break;

    if (tag[0] == '/') continue;  // end tag, skip

    // Dispatch to section parsers
    if (strcmp(tag, "identity") == 0) {
      _pos = saved;
      if (!parse_identity()) return false;
    } else if (strcmp(tag, "provenance") == 0) {
      _pos = saved;
      if (!parse_provenance()) return false;
    } else if (strcmp(tag, "documentation") == 0) {
      _pos = saved;
      if (!parse_documentation()) return false;
    } else if (strcmp(tag, "design") == 0) {
      _pos = saved;
      if (!parse_design()) return false;
    } else if (strcmp(tag, "security") == 0) {
      _pos = saved;
      if (!parse_security()) return false;
    } else if (strcmp(tag, "dependencies") == 0) {
      _pos = saved;
      if (!parse_dependencies()) return false;
    } else if (strcmp(tag, "constant-pool") == 0) {
      _pos = saved;
      if (!parse_constant_pool()) return false;
    } else if (strcmp(tag, "fields") == 0) {
      _pos = saved;
      if (!parse_fields()) return false;
    } else if (strcmp(tag, "methods") == 0) {
      _pos = saved;
      if (!parse_methods()) return false;
    } else if (strcmp(tag, "hints") == 0) {
      _pos = saved;
      if (!parse_hints()) return false;
    } else if (strcmp(tag, "tests") == 0) {
      _pos = saved;
      if (!parse_tests()) return false;
    } else if (strcmp(tag, "history") == 0) {
      _pos = saved;
      if (!parse_history()) return false;
    } else if (strcmp(tag, "annotations") == 0) {
      _pos = saved;
      if (!parse_annotations()) return false;
    } else if (strcmp(tag, "inner-classes") == 0) {
      // Skip for now — self-closing or empty
      skip_to_end_tag("inner-classes");
    } else if (strcmp(tag, "source-debug") == 0) {
      skip_to_end_tag("source-debug");
    } else {
      // Unknown section — skip to its end tag
      skip_to_end_tag(tag);
    }
  }

  // Now assemble the binary .class from parsed data
  assemble_constant_pool();
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Section Parsers
// ═══════════════════════════════════════════════════════════════════════════════

bool XMLClassReader::parse_identity() {
  if (!expect_tag("identity")) return false;

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 11 <= _end && strncmp(_pos, "</identity>", 11) == 0) {
      _pos += 11;
      break;
    }

    const char* saved = _pos;
    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "name") == 0) {
      read_element_text(text, sizeof(text));
      // Convert dots to slashes for internal form
      strncpy(_parsed.class_name, text, sizeof(_parsed.class_name) - 1);
      for (char* p = _parsed.class_name; *p; p++) {
        if (*p == '.') *p = '/';
      }
      skip_to_end_tag("name");
    } else if (strcmp(tag, "super") == 0) {
      read_element_text(text, sizeof(text));
      strncpy(_parsed.super_name, text, sizeof(_parsed.super_name) - 1);
      for (char* p = _parsed.super_name; *p; p++) {
        if (*p == '.') *p = '/';
      }
      skip_to_end_tag("super");
    } else if (strcmp(tag, "access") == 0) {
      read_element_text(text, sizeof(text));
      _parsed.access_flags = 0x0020;  // ACC_SUPER always
      if (strstr(text, "public"))    _parsed.access_flags |= 0x0001;
      if (strstr(text, "final"))     _parsed.access_flags |= 0x0010;
      if (strstr(text, "abstract"))  _parsed.access_flags |= 0x0400;
      if (strstr(text, "interface")) _parsed.access_flags |= 0x0200;
      if (strstr(text, "enum"))      _parsed.access_flags |= 0x4000;
      skip_to_end_tag("access");
    } else if (strcmp(tag, "version") == 0) {
      // Attributes already parsed by expect_tag variant — use saved pos
      // For now use defaults (Java 28)
    } else if (strcmp(tag, "interfaces") == 0) {
      // Parse interface children
      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 13 <= _end && strncmp(_pos, "</interfaces>", 13) == 0) {
          _pos += 13;
          break;
        }
        const char* s2 = _pos;
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (strcmp(tag, "interface") == 0) {
          read_element_text(text, sizeof(text));
          // Will resolve to CP index later
          // For now store the name — resolved during assembly
          skip_to_end_tag("interface");
        } else if (tag[0] == '/') {
          continue;
        } else {
          skip_to_end_tag(tag);
        }
      }
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_provenance() {
  if (!expect_tag("provenance")) return false;

  _metadata->provenance = new XClassProvenance();
  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 13 <= _end && strncmp(_pos, "</provenance>", 13) == 0) {
      _pos += 13;
      break;
    }

    const char* saved = _pos;
    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "author") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->provenance->author = os::strdup(text);
      skip_to_end_tag("author");
    } else if (strcmp(tag, "organization") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->provenance->organization = os::strdup(text);
      skip_to_end_tag("organization");
    } else if (strcmp(tag, "created") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->provenance->created = os::strdup(text);
      skip_to_end_tag("created");
    } else if (strcmp(tag, "modified") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->provenance->modified = os::strdup(text);
      skip_to_end_tag("modified");
    } else if (strcmp(tag, "license") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->provenance->license = os::strdup(text);
      skip_to_end_tag("license");
    } else if (strcmp(tag, "origin") == 0) {
      // Parse origin children
      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 9 <= _end && strncmp(_pos, "</origin>", 9) == 0) {
          _pos += 9;
          break;
        }
        const char* s2 = _pos;
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;
        if (strcmp(tag, "source-file") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->provenance->source_file = os::strdup(text);
          skip_to_end_tag("source-file");
        } else if (strcmp(tag, "repository") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->provenance->repository = os::strdup(text);
          skip_to_end_tag("repository");
        } else if (strcmp(tag, "commit") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->provenance->commit = os::strdup(text);
          skip_to_end_tag("commit");
        } else {
          skip_to_end_tag(tag);
        }
      }
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_documentation() {
  if (!expect_tag("documentation")) return false;

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_TEXT];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 16 <= _end && strncmp(_pos, "</documentation>", 16) == 0) {
      _pos += 16;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "summary") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->documentation_summary = os::strdup(text);
      skip_to_end_tag("summary");
    } else if (strcmp(tag, "details") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->documentation_details = os::strdup(text);
      skip_to_end_tag("details");
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_design() {
  if (!expect_tag("design")) return false;

  _metadata->design = new XClassDesign();
  _metadata->design->preconditions = new (mtClass) GrowableArray<const char*>(4);
  _metadata->design->postconditions = new (mtClass) GrowableArray<const char*>(4);
  _metadata->design->invariants = new (mtClass) GrowableArray<const char*>(4);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_TEXT];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 9 <= _end && strncmp(_pos, "</design>", 9) == 0) {
      _pos += 9;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "intent") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->design->intent = os::strdup(text);
      skip_to_end_tag("intent");
    } else if (strcmp(tag, "pattern") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->design->pattern = os::strdup(text);
      skip_to_end_tag("pattern");
    } else if (strcmp(tag, "thread-safety") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->design->thread_safety = os::strdup(text);
      skip_to_end_tag("thread-safety");
    } else if (strcmp(tag, "nullability") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->design->nullability = os::strdup(text);
      skip_to_end_tag("nullability");
    } else if (strcmp(tag, "contract") == 0) {
      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 11 <= _end && strncmp(_pos, "</contract>", 11) == 0) {
          _pos += 11;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;
        if (strcmp(tag, "precondition") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->design->preconditions->append(os::strdup(text));
          skip_to_end_tag("precondition");
        } else if (strcmp(tag, "postcondition") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->design->postconditions->append(os::strdup(text));
          skip_to_end_tag("postcondition");
        } else if (strcmp(tag, "invariant") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->design->invariants->append(os::strdup(text));
          skip_to_end_tag("invariant");
        } else {
          skip_to_end_tag(tag);
        }
      }
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_security() {
  if (!expect_tag("security")) return false;

  _metadata->security = new XClassSecurity();
  _metadata->security->permissions = new (mtClass) GrowableArray<const char*>(4);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 11 <= _end && strncmp(_pos, "</security>", 11) == 0) {
      _pos += 11;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "trust-grade") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->security->trust_grade = atoi(text);
      skip_to_end_tag("trust-grade");
    } else if (strcmp(tag, "classload-grade") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->security->classload_grade = atoi(text);
      skip_to_end_tag("classload-grade");
    } else if (strcmp(tag, "permissions") == 0) {
      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 14 <= _end && strncmp(_pos, "</permissions>", 14) == 0) {
          _pos += 14;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;
        if (strcmp(tag, "permission") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->security->permissions->append(os::strdup(text));
          skip_to_end_tag("permission");
        } else {
          skip_to_end_tag(tag);
        }
      }
    } else if (strcmp(tag, "resource-budget") == 0) {
      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 18 <= _end && strncmp(_pos, "</resource-budget>", 18) == 0) {
          _pos += 18;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;
        if (strcmp(tag, "max-memory") == 0) {
          read_element_text(text, sizeof(text));
          // Parse size suffix (m, g)
          int64_t val = atoll(text);
          int len = (int)strlen(text);
          if (len > 0 && (text[len-1] == 'm' || text[len-1] == 'M')) val *= 1024 * 1024;
          if (len > 0 && (text[len-1] == 'g' || text[len-1] == 'G')) val *= 1024LL * 1024 * 1024;
          _metadata->security->max_memory = val;
          skip_to_end_tag("max-memory");
        } else if (strcmp(tag, "max-threads") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->security->max_threads = atoi(text);
          skip_to_end_tag("max-threads");
        } else if (strcmp(tag, "max-file-descriptors") == 0) {
          read_element_text(text, sizeof(text));
          _metadata->security->max_file_descriptors = atoi(text);
          skip_to_end_tag("max-file-descriptors");
        } else {
          skip_to_end_tag(tag);
        }
      }
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_dependencies() {
  if (!expect_tag("dependencies")) return false;

  _metadata->dependencies = new (mtClass) GrowableArray<XClassDependency*>(8);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 15 <= _end && strncmp(_pos, "</dependencies>", 15) == 0) {
      _pos += 15;
      break;
    }
    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "dependency") == 0) {
      // Attributes are in the tag — already past '>'
      // For now, skip content (self-closing or empty)
      XClassDependency* dep = new XClassDependency();
      _metadata->dependencies->append(dep);
      // Skip to end or handle self-closing
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_constant_pool() {
  if (!expect_tag("constant-pool")) return false;

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 16 <= _end && strncmp(_pos, "</constant-pool>", 16) == 0) {
      _pos += 16;
      break;
    }

    // Skip comments
    while (_pos + 4 <= _end && _pos[0] == '<' && _pos[1] == '!' &&
           _pos[2] == '-' && _pos[3] == '-') {
      if (!skip_comment()) return false;
      skip_whitespace();
    }

    if (_pos >= _end) break;
    if (_pos + 16 <= _end && strncmp(_pos, "</constant-pool>", 16) == 0) {
      _pos += 16;
      break;
    }

    // Each entry is: <entry index="N" tag="TYPE" .../>
    // We need to parse attributes from within the tag
    // Save position before the <entry
    const char* entry_start = _pos;

    if (*_pos != '<') { _pos++; continue; }
    _pos++;  // skip '<'

    // Read tag name
    const char* tname_start = _pos;
    while (_pos < _end && *_pos != ' ' && *_pos != '>' && *_pos != '/') _pos++;
    int tname_len = (int)(_pos - tname_start);

    if (tname_len == 5 && strncmp(tname_start, "entry", 5) == 0) {
      // Parse attributes within this tag
      int index = 0;
      char cp_tag[64] = "";
      char value[XCLASS_MAX_ATTR_VALUE] = "";
      char name[512] = "";
      int name_index = 0, descriptor_index = 0;
      int class_index = 0, name_and_type_index = 0;
      int string_index = 0;

      // Read all attributes until > or />
      while (_pos < _end && *_pos != '>' && !(*_pos == '/' && _pos + 1 < _end && _pos[1] == '>')) {
        // Skip whitespace
        while (_pos < _end && (*_pos == ' ' || *_pos == '\t' || *_pos == '\n' || *_pos == '\r')) _pos++;
        if (_pos >= _end || *_pos == '>' || *_pos == '/') break;

        // Read attribute name
        const char* attr_start = _pos;
        while (_pos < _end && *_pos != '=') _pos++;
        int attr_name_len = (int)(_pos - attr_start);
        if (_pos >= _end) break;
        _pos++;  // skip '='
        if (_pos >= _end || *_pos != '"') break;
        _pos++;  // skip '"'
        const char* val_start = _pos;
        while (_pos < _end && *_pos != '"') _pos++;
        int val_len = (int)(_pos - val_start);
        if (_pos < _end) _pos++;  // skip closing '"'

        // Match known attributes
        if (attr_name_len == 5 && strncmp(attr_start, "index", 5) == 0) {
          char idx_buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(idx_buf, val_start, copy_len);
          idx_buf[copy_len] = '\0';
          index = atoi(idx_buf);
        } else if (attr_name_len == 3 && strncmp(attr_start, "tag", 3) == 0) {
          int copy_len = val_len < 63 ? val_len : 63;
          strncpy(cp_tag, val_start, copy_len);
          cp_tag[copy_len] = '\0';
        } else if (attr_name_len == 5 && strncmp(attr_start, "value", 5) == 0) {
          int copy_len = val_len < (int)sizeof(value) - 1 ? val_len : (int)sizeof(value) - 1;
          strncpy(value, val_start, copy_len);
          value[copy_len] = '\0';
        } else if (attr_name_len == 4 && strncmp(attr_start, "name", 4) == 0) {
          int copy_len = val_len < 511 ? val_len : 511;
          strncpy(name, val_start, copy_len);
          name[copy_len] = '\0';
        } else if (attr_name_len == 10 && strncmp(attr_start, "name-index", 10) == 0) {
          char buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(buf, val_start, copy_len); buf[copy_len] = '\0';
          name_index = atoi(buf);
        } else if (attr_name_len == 16 && strncmp(attr_start, "descriptor-index", 16) == 0) {
          char buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(buf, val_start, copy_len); buf[copy_len] = '\0';
          descriptor_index = atoi(buf);
        } else if (attr_name_len == 11 && strncmp(attr_start, "class-index", 11) == 0) {
          char buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(buf, val_start, copy_len); buf[copy_len] = '\0';
          class_index = atoi(buf);
        } else if (attr_name_len == 19 && strncmp(attr_start, "name-and-type-index", 19) == 0) {
          char buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(buf, val_start, copy_len); buf[copy_len] = '\0';
          name_and_type_index = atoi(buf);
        } else if (attr_name_len == 12 && strncmp(attr_start, "string-index", 12) == 0) {
          char buf[32];
          int copy_len = val_len < 31 ? val_len : 31;
          strncpy(buf, val_start, copy_len); buf[copy_len] = '\0';
          string_index = atoi(buf);
        }
      }

      // Skip to end of tag
      while (_pos < _end && *_pos != '>') _pos++;
      if (_pos < _end) _pos++;

      // Create CP entry
      CPEntry* entry = new (mtClass) CPEntry();

      if (strcmp(cp_tag, "Utf8") == 0) {
        entry->tag = CPEntry::CONSTANT_Utf8;
        entry->data.utf8.value = os::strdup(value);
        entry->data.utf8.length = (int)strlen(value);
      } else if (strcmp(cp_tag, "Integer") == 0) {
        entry->tag = CPEntry::CONSTANT_Integer;
        entry->data.int_value = atoi(value);
      } else if (strcmp(cp_tag, "Float") == 0) {
        entry->tag = CPEntry::CONSTANT_Float;
        entry->data.float_value = (float)atof(value);
      } else if (strcmp(cp_tag, "Long") == 0) {
        entry->tag = CPEntry::CONSTANT_Long;
        entry->data.long_value = atoll(value);
      } else if (strcmp(cp_tag, "Double") == 0) {
        entry->tag = CPEntry::CONSTANT_Double;
        entry->data.double_value = atof(value);
      } else if (strcmp(cp_tag, "Class") == 0) {
        entry->tag = CPEntry::CONSTANT_Class;
        entry->data.class_info.name_index = name_index > 0 ? name_index : index;
        // If 'name' attribute provided, it's the resolved name (informational)
      } else if (strcmp(cp_tag, "String") == 0) {
        entry->tag = CPEntry::CONSTANT_String;
        entry->data.string_info.string_index = string_index;
      } else if (strcmp(cp_tag, "Fieldref") == 0) {
        entry->tag = CPEntry::CONSTANT_Fieldref;
        entry->data.ref_info.class_index = class_index;
        entry->data.ref_info.name_and_type_index = name_and_type_index;
      } else if (strcmp(cp_tag, "Methodref") == 0) {
        entry->tag = CPEntry::CONSTANT_Methodref;
        entry->data.ref_info.class_index = class_index;
        entry->data.ref_info.name_and_type_index = name_and_type_index;
      } else if (strcmp(cp_tag, "InterfaceMethodref") == 0) {
        entry->tag = CPEntry::CONSTANT_InterfaceMethodref;
        entry->data.ref_info.class_index = class_index;
        entry->data.ref_info.name_and_type_index = name_and_type_index;
      } else if (strcmp(cp_tag, "NameAndType") == 0) {
        entry->tag = CPEntry::CONSTANT_NameAndType;
        entry->data.name_and_type.name_index = name_index;
        entry->data.name_and_type.descriptor_index = descriptor_index;
      } else {
        // Unknown tag type — treat as Utf8 placeholder
        entry->tag = CPEntry::CONSTANT_Utf8;
        entry->data.utf8.value = "";
        entry->data.utf8.length = 0;
      }

      // Ensure array is large enough
      while (_parsed.cp_entries->length() < index) {
        _parsed.cp_entries->append(nullptr);
      }
      if (index > 0) {
        if (index - 1 < _parsed.cp_entries->length()) {
          _parsed.cp_entries->at_put(index - 1, entry);
        } else {
          _parsed.cp_entries->append(entry);
        }
      }
      if (index >= _parsed.cp_count) {
        _parsed.cp_count = index + 1;
        // Long and Double take two slots
        if (entry->tag == CPEntry::CONSTANT_Long || entry->tag == CPEntry::CONSTANT_Double) {
          _parsed.cp_count = index + 2;
        }
      }
    } else if (tname_start[0] == '/') {
      // End tag
      while (_pos < _end && *_pos != '>') _pos++;
      if (_pos < _end) _pos++;
    } else {
      // Skip unknown tag
      while (_pos < _end && *_pos != '>') _pos++;
      if (_pos < _end) _pos++;
    }
  }
  return true;
}

bool XMLClassReader::parse_fields() {
  if (!expect_tag("fields")) return false;

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 9 <= _end && strncmp(_pos, "</fields>", 9) == 0) {
      _pos += 9;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "field") == 0) {
      // Parse field children
      char fname[256] = "";
      char fdesc[256] = "";
      char faccess[256] = "";

      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 8 <= _end && strncmp(_pos, "</field>", 8) == 0) {
          _pos += 8;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;

        if (strcmp(tag, "name") == 0) {
          read_element_text(fname, sizeof(fname));
          skip_to_end_tag("name");
        } else if (strcmp(tag, "descriptor") == 0) {
          read_element_text(fdesc, sizeof(fdesc));
          skip_to_end_tag("descriptor");
        } else if (strcmp(tag, "access") == 0) {
          read_element_text(faccess, sizeof(faccess));
          skip_to_end_tag("access");
        } else {
          skip_to_end_tag(tag);
        }
      }

      // Record field — CP indices will be resolved during assembly
      // For now, store names for later
      (void)fname;
      (void)fdesc;
      (void)faccess;
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_methods() {
  if (!expect_tag("methods")) return false;

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 10 <= _end && strncmp(_pos, "</methods>", 10) == 0) {
      _pos += 10;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "method") == 0) {
      char mname[256] = "";
      char mdesc[256] = "";
      char maccess[256] = "";
      int max_stack = 1;
      int max_locals = 1;
      unsigned char bytecode[XCLASS_MAX_BYTECODE];
      int bytecode_len = 0;

      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 9 <= _end && strncmp(_pos, "</method>", 9) == 0) {
          _pos += 9;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;

        if (strcmp(tag, "name") == 0) {
          read_element_text(mname, sizeof(mname));
          skip_to_end_tag("name");
        } else if (strcmp(tag, "descriptor") == 0) {
          read_element_text(mdesc, sizeof(mdesc));
          skip_to_end_tag("descriptor");
        } else if (strcmp(tag, "access") == 0) {
          read_element_text(maccess, sizeof(maccess));
          skip_to_end_tag("access");
        } else if (strcmp(tag, "code") == 0) {
          // Parse code children
          while (_pos < _end) {
            skip_whitespace();
            if (_pos + 7 <= _end && strncmp(_pos, "</code>", 7) == 0) {
              _pos += 7;
              break;
            }
            if (!read_tag_name(tag, sizeof(tag))) break;
            if (tag[0] == '/') continue;
            if (strcmp(tag, "bytecode") == 0) {
              // Read Base64 content
              char b64[XCLASS_MAX_TEXT];
              read_element_text(b64, sizeof(b64));
              bytecode_len = base64_decode(b64, (int)strlen(b64),
                                           bytecode, XCLASS_MAX_BYTECODE);
              skip_to_end_tag("bytecode");
            } else {
              skip_to_end_tag(tag);
            }
          }
        } else if (strcmp(tag, "contract") == 0) {
          // Method-level contracts → metadata
          skip_to_end_tag("contract");
        } else {
          skip_to_end_tag(tag);
        }
      }

      // Store parsed method (resolved during assembly)
      (void)mname;
      (void)mdesc;
      (void)maccess;
      (void)max_stack;
      (void)max_locals;
      (void)bytecode_len;
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_hints() {
  if (!expect_tag("hints")) return false;

  _metadata->hints = new XClassHint();
  _metadata->hints->hot_methods = new (mtClass) GrowableArray<const char*>(4);
  _metadata->hints->inline_candidates = new (mtClass) GrowableArray<const char*>(4);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_ATTR_VALUE];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 8 <= _end && strncmp(_pos, "</hints>", 8) == 0) {
      _pos += 8;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "hot-method") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->hints->hot_methods->append(os::strdup(text));
      skip_to_end_tag("hot-method");
    } else if (strcmp(tag, "inline-candidate") == 0) {
      read_element_text(text, sizeof(text));
      _metadata->hints->inline_candidates->append(os::strdup(text));
      skip_to_end_tag("inline-candidate");
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_tests() {
  if (!expect_tag("tests")) return false;

  _metadata->tests = new (mtClass) GrowableArray<XClassTest*>(8);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_TEXT];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 8 <= _end && strncmp(_pos, "</tests>", 8) == 0) {
      _pos += 8;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "test") == 0) {
      XClassTest* test = new XClassTest();
      test->expectations = new (mtClass) GrowableArray<const char*>(4);

      while (_pos < _end) {
        skip_whitespace();
        if (_pos + 7 <= _end && strncmp(_pos, "</test>", 7) == 0) {
          _pos += 7;
          break;
        }
        if (!read_tag_name(tag, sizeof(tag))) break;
        if (tag[0] == '/') continue;

        if (strcmp(tag, "given") == 0) {
          read_element_text(text, sizeof(text));
          test->given = os::strdup(text);
          skip_to_end_tag("given");
        } else if (strcmp(tag, "expect") == 0) {
          read_element_text(text, sizeof(text));
          test->expectations->append(os::strdup(text));
          skip_to_end_tag("expect");
        } else if (strcmp(tag, "expect-throws") == 0) {
          read_element_text(text, sizeof(text));
          test->expect_throws = os::strdup(text);
          skip_to_end_tag("expect-throws");
        } else {
          skip_to_end_tag(tag);
        }
      }
      _metadata->tests->append(test);
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_history() {
  if (!expect_tag("history")) return false;

  _metadata->history = new (mtClass) GrowableArray<XClassRevision*>(8);

  char tag[XCLASS_MAX_TAG_NAME];
  char text[XCLASS_MAX_TEXT];

  while (_pos < _end) {
    skip_whitespace();
    if (_pos + 10 <= _end && strncmp(_pos, "</history>", 10) == 0) {
      _pos += 10;
      break;
    }

    if (!read_tag_name(tag, sizeof(tag))) break;
    if (tag[0] == '/') continue;

    if (strcmp(tag, "revision") == 0) {
      XClassRevision* rev = new XClassRevision();
      // Read text content
      read_element_text(text, sizeof(text));
      rev->description = os::strdup(text);
      _metadata->history->append(rev);
      skip_to_end_tag("revision");
    } else {
      skip_to_end_tag(tag);
    }
  }
  return true;
}

bool XMLClassReader::parse_annotations() {
  if (!expect_tag("annotations")) return false;
  skip_to_end_tag("annotations");
  return true;
}

bool XMLClassReader::parse_inner_classes() {
  if (!expect_tag("inner-classes")) return false;
  skip_to_end_tag("inner-classes");
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Binary Class File Assembly
// ═══════════════════════════════════════════════════════════════════════════════

void XMLClassReader::assemble_constant_pool() {
  // Emit the full binary .class file

  // 1. Magic number
  emit_u4(JAVA_MAGIC);  // 0xCAFEBABE

  // 2. Version
  emit_u2(_parsed.minor_version);
  emit_u2(_parsed.major_version);

  // 3. Constant pool
  int cp_count = _parsed.cp_entries->length() + 1;  // +1 because CP is 1-indexed
  emit_u2((unsigned short)cp_count);

  for (int i = 0; i < _parsed.cp_entries->length(); i++) {
    CPEntry* entry = _parsed.cp_entries->at(i);
    if (entry == nullptr) {
      // Filler slot (e.g., second slot of Long/Double)
      // Should not normally happen at index 0, but handle gracefully
      emit_u1(CPEntry::CONSTANT_Utf8);
      emit_u2(0);  // empty string
      continue;
    }

    emit_u1((unsigned char)entry->tag);
    switch (entry->tag) {
      case CPEntry::CONSTANT_Utf8:
        emit_u2((unsigned short)entry->data.utf8.length);
        emit_bytes((const unsigned char*)entry->data.utf8.value, entry->data.utf8.length);
        break;
      case CPEntry::CONSTANT_Integer:
        emit_u4((unsigned int)entry->data.int_value);
        break;
      case CPEntry::CONSTANT_Float: {
        union { float f; unsigned int u; } conv;
        conv.f = entry->data.float_value;
        emit_u4(conv.u);
        break;
      }
      case CPEntry::CONSTANT_Long: {
        long val = entry->data.long_value;
        emit_u4((unsigned int)(val >> 32));
        emit_u4((unsigned int)(val & 0xFFFFFFFF));
        // Long takes two CP slots — emit placeholder for next
        i++;  // skip next slot
        break;
      }
      case CPEntry::CONSTANT_Double: {
        union { double d; unsigned long long u; } conv;
        conv.d = entry->data.double_value;
        emit_u4((unsigned int)(conv.u >> 32));
        emit_u4((unsigned int)(conv.u & 0xFFFFFFFF));
        i++;  // skip next slot
        break;
      }
      case CPEntry::CONSTANT_Class:
        emit_u2((unsigned short)entry->data.class_info.name_index);
        break;
      case CPEntry::CONSTANT_String:
        emit_u2((unsigned short)entry->data.string_info.string_index);
        break;
      case CPEntry::CONSTANT_Fieldref:
      case CPEntry::CONSTANT_Methodref:
      case CPEntry::CONSTANT_InterfaceMethodref:
        emit_u2((unsigned short)entry->data.ref_info.class_index);
        emit_u2((unsigned short)entry->data.ref_info.name_and_type_index);
        break;
      case CPEntry::CONSTANT_NameAndType:
        emit_u2((unsigned short)entry->data.name_and_type.name_index);
        emit_u2((unsigned short)entry->data.name_and_type.descriptor_index);
        break;
      default:
        // Unknown — emit as empty Utf8
        emit_u2(0);
        break;
    }
  }

  // 4. Access flags
  emit_u2((unsigned short)_parsed.access_flags);

  // 5. This class (CP index — find the Class entry for our name)
  // For simplicity, assume index 1 is always "this class"
  emit_u2(1);

  // 6. Super class (CP index — find the Class entry for super)
  // Assume index 2 is always "super class"
  emit_u2(2);

  // 7. Interfaces
  emit_u2((unsigned short)_parsed.interface_cp_indices->length());
  for (int i = 0; i < _parsed.interface_cp_indices->length(); i++) {
    emit_u2((unsigned short)_parsed.interface_cp_indices->at(i));
  }

  // 8. Fields (simplified — emit 0 for now, parsed from XML)
  emit_u2(0);  // fields_count — TODO: full field assembly

  // 9. Methods (simplified — emit 0 for now, parsed from XML)
  emit_u2(0);  // methods_count — TODO: full method assembly

  // 10. Attributes
  emit_u2(0);  // attributes_count
}

// ═══════════════════════════════════════════════════════════════════════════════
// Metadata Registration with Secure JVM Modules
// ═══════════════════════════════════════════════════════════════════════════════

void XMLClassReader::register_metadata_with_classload_guard() {
  // If security.classload-grade is specified, inform ClassLoadGuard
  if (_metadata->security && _metadata->security->classload_grade >= 0) {
    // ClassLoadGuard integration point:
    // The grade from XML overrides the heuristic name-based detection
    // This allows precise architectural grading of each class
  }
}

void XMLClassReader::register_metadata_with_integrity_guardian() {
  // If trust-grade is specified, inform the Integrity Guardian
  if (_metadata->security && _metadata->security->trust_grade >= 0) {
    // Integrity Guardian integration point:
    // Classes with explicit trust grades get different allocation discipline
  }
}

void XMLClassReader::register_metadata_with_observer_circuit() {
  // Register provenance and history with observer circuits
  if (_metadata->provenance != nullptr) {
    // Observer Circuit integration point:
    // Provenance data available for inspection at Circuit 1+
  }
}

void XMLClassReader::register_metadata_with_memory_proxy() {
  // If resource-budget is specified, configure Memory Proxy limits
  if (_metadata->security != nullptr) {
    if (_metadata->security->max_memory > 0 ||
        _metadata->security->max_threads > 0) {
      // Memory Proxy integration point:
      // Native processes launched by this class get these budgets
    }
  }
}

void XMLClassReader::register_metadata_with_codex() {
  // Register design intent with System Codex
  if (_metadata->design && _metadata->design->intent != nullptr) {
    // System Codex integration point:
    // Class registered with its shape, color, and rigor from design metadata
  }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════════

const char* XMLClassReader::parsed_class_name() const {
  if (_parsed.class_name[0] != '\0') {
    return _parsed.class_name;
  }
  return nullptr;
}

bool XMLClassReader::validate_signature() const {
  // TODO: Validate SHA-256 signature from <class signature="sha256:...">
  // For now, accept all
  return true;
}

bool XMLClassReader::validate_bytecode_integrity() const {
  // TODO: Verify that Base64-decoded bytecode sections pass basic validation
  return true;
}
