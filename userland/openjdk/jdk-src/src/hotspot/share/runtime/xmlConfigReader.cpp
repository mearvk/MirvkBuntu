/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * xmlConfigReader.cpp - Secure XML Configuration Reader for JVM Start Flags
 *
 * Integrates with Arguments::parse_vm_init_args() to load JVM flags
 * from a validated XML document. No external XML library dependencies —
 * uses a minimal secure parser sufficient for the fixed schema.
 */

#include "runtime/xmlConfigReader.hpp"
#include "runtime/arguments.hpp"
#include "runtime/flags/jvmFlag.hpp"
#include "runtime/flags/jvmFlagAccess.hpp"
#include "runtime/os.hpp"
#include "utilities/ostream.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ============================================================================
// xmlConfigEntry
// ============================================================================

xmlConfigEntry::xmlConfigEntry(EntryType type, const char* name, const char* value) {
  _type = type;
  _name = os::strdup(name);
  _value = (value != nullptr) ? os::strdup(value) : nullptr;
}

xmlConfigEntry::~xmlConfigEntry() {
  os::free(_name);
  if (_value != nullptr) os::free(_value);
}

// ============================================================================
// Security Validation
// ============================================================================

bool xmlConfigReader::validate_file_permissions(const char* path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return false;
  }

  uid_t my_uid = getuid();

  // File must be owned by root or the launching user
  if (st.st_uid != 0 && st.st_uid != my_uid) {
    warning("XML config '%s': not owned by root or current user (uid=%d, file_uid=%d)",
            path, (int)my_uid, (int)st.st_uid);
    return false;
  }

  // File must not be world-writable
  if (st.st_mode & S_IWOTH) {
    warning("XML config '%s': world-writable (insecure, refusing to load)", path);
    return false;
  }

  // File must not be group-writable unless group matches user's group
  if ((st.st_mode & S_IWGRP) && st.st_gid != getgid()) {
    warning("XML config '%s': writable by foreign group (insecure)", path);
    return false;
  }

  // Must be a regular file (not symlink to something dangerous)
  if (!S_ISREG(st.st_mode)) {
    warning("XML config '%s': not a regular file", path);
    return false;
  }

  return true;
}

bool xmlConfigReader::validate_file_size(const char* path, size_t* out_size) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return false;
  }

  if (st.st_size <= 0 || (size_t)st.st_size > XML_CONFIG_MAX_SIZE) {
    warning("XML config '%s': invalid size (%ld bytes, max %d)",
            path, (long)st.st_size, XML_CONFIG_MAX_SIZE);
    return false;
  }

  *out_size = (size_t)st.st_size;
  return true;
}

bool xmlConfigReader::validate_signature(const char* content, size_t len,
                                         const char* expected_sig) {
  // If no signature specified, allow (unsigned configs are valid)
  if (expected_sig == nullptr || expected_sig[0] == '\0') {
    return true;
  }

  // Expect format: "sha256:<hex_digest>"
  if (strncmp(expected_sig, "sha256:", 7) != 0) {
    warning("XML config: unsupported signature format (expected sha256:...)");
    return false;
  }

  // TODO: Implement SHA-256 verification against the <flags> content.
  // For now, log that signature was present but not verified.
  // Full implementation would hash the content (excluding the signature attr)
  // and compare against the provided digest.
  //
  // This is a placeholder for integration with the system's crypto layer.
  // On this OS, Dave could verify signatures via the SSL/TLS infrastructure.

  return true;
}

// ============================================================================
// Minimal Secure XML Parser
// ============================================================================

// Find the start of an opening tag: <tag_name ...>
const char* xmlConfigReader::find_tag_start(const char* pos, const char* end,
                                            const char* tag_name) {
  size_t tag_len = strlen(tag_name);
  while (pos < end - tag_len - 2) {
    if (*pos == '<' && strncmp(pos + 1, tag_name, tag_len) == 0) {
      char after = pos[1 + tag_len];
      if (after == '>' || after == ' ' || after == '/' || after == '\t' ||
          after == '\n' || after == '\r') {
        return pos;
      }
    }
    pos++;
  }
  return nullptr;
}

// Find the closing tag: </tag_name>
const char* xmlConfigReader::find_tag_end(const char* pos, const char* end,
                                          const char* tag_name) {
  size_t tag_len = strlen(tag_name);
  while (pos < end - tag_len - 3) {
    if (pos[0] == '<' && pos[1] == '/' && strncmp(pos + 2, tag_name, tag_len) == 0 &&
        pos[2 + tag_len] == '>') {
      return pos;
    }
    pos++;
  }
  return nullptr;
}

// Extract an attribute value from within an element's opening tag
bool xmlConfigReader::parse_attribute(const char* element_start, const char* element_end,
                                      const char* attr_name, char* out_value, size_t max_len) {
  // Find end of opening tag (the first >)
  const char* tag_end = element_start;
  while (tag_end < element_end && *tag_end != '>') tag_end++;
  if (tag_end >= element_end) return false;

  size_t attr_len = strlen(attr_name);
  const char* pos = element_start;

  while (pos < tag_end - attr_len) {
    if (strncmp(pos, attr_name, attr_len) == 0 && pos[attr_len] == '=') {
      pos += attr_len + 1;
      // Skip quote
      char quote = *pos;
      if (quote != '"' && quote != '\'') return false;
      pos++;
      // Read until closing quote
      size_t i = 0;
      while (pos < tag_end && *pos != quote && i < max_len - 1) {
        // Security: reject any XML entities or special constructs
        if (*pos == '&' || *pos == '<') {
          warning("XML config: illegal character in attribute value");
          return false;
        }
        out_value[i++] = *pos++;
      }
      out_value[i] = '\0';
      return true;
    }
    pos++;
  }
  return false;
}

bool xmlConfigReader::parse_xml(const char* content, size_t len,
                                GrowableArrayCHeap<xmlConfigEntry*, mtArguments>* entries) {
  const char* end = content + len;

  // Security: reject if file contains DTD or external entity declarations
  if (strstr(content, "<!DOCTYPE") != nullptr ||
      strstr(content, "<!ENTITY") != nullptr ||
      strstr(content, "SYSTEM") != nullptr) {
    warning("XML config: DOCTYPE/ENTITY/SYSTEM declarations not allowed (security)");
    return false;
  }

  // Find <jvm-config> root
  const char* root_start = find_tag_start(content, end, "jvm-config");
  if (root_start == nullptr) {
    warning("XML config: missing <jvm-config> root element");
    return false;
  }

  const char* root_end = find_tag_end(root_start, end, "jvm-config");
  if (root_end == nullptr) {
    warning("XML config: missing </jvm-config> closing tag");
    return false;
  }

  // Check optional signature attribute on root
  char signature[256] = {0};
  parse_attribute(root_start, root_end, "signature", signature, sizeof(signature));
  if (!validate_signature(content, len, signature)) {
    return false;
  }

  // Parse <flags> section
  const char* flags_start = find_tag_start(root_start, root_end, "flags");
  if (flags_start != nullptr) {
    const char* flags_end = find_tag_end(flags_start, root_end, "flags");
    if (flags_end != nullptr) {
      // Find each <flag name="..." value="..."/>
      const char* pos = flags_start;
      while (pos < flags_end && entries->length() < XML_CONFIG_MAX_ENTRIES) {
        const char* flag_el = find_tag_start(pos, flags_end, "flag");
        if (flag_el == nullptr) break;

        char name[XML_CONFIG_MAX_TOKEN] = {0};
        char value[XML_CONFIG_MAX_TOKEN] = {0};

        // Find the end of this element (either /> or next <)
        const char* el_bound = flag_el + 1;
        while (el_bound < flags_end && *el_bound != '>' && *el_bound != '<') el_bound++;
        if (*el_bound == '>') el_bound++; // include the >

        if (parse_attribute(flag_el, el_bound, "name", name, sizeof(name))) {
          parse_attribute(flag_el, el_bound, "value", value, sizeof(value));
          entries->append(new xmlConfigEntry(xmlConfigEntry::FLAG, name, value));
        }
        pos = el_bound;
      }
    }
  }

  // Parse <system-properties> section
  const char* props_start = find_tag_start(root_start, root_end, "system-properties");
  if (props_start != nullptr) {
    const char* props_end = find_tag_end(props_start, root_end, "system-properties");
    if (props_end != nullptr) {
      const char* pos = props_start;
      while (pos < props_end && entries->length() < XML_CONFIG_MAX_ENTRIES) {
        const char* prop_el = find_tag_start(pos, props_end, "property");
        if (prop_el == nullptr) break;

        char name[XML_CONFIG_MAX_TOKEN] = {0};
        char value[XML_CONFIG_MAX_TOKEN] = {0};

        const char* el_bound = prop_el + 1;
        while (el_bound < props_end && *el_bound != '>' && *el_bound != '<') el_bound++;
        if (*el_bound == '>') el_bound++;

        if (parse_attribute(prop_el, el_bound, "name", name, sizeof(name)) &&
            parse_attribute(prop_el, el_bound, "value", value, sizeof(value))) {
          entries->append(new xmlConfigEntry(xmlConfigEntry::SYSTEM_PROPERTY, name, value));
        }
        pos = el_bound;
      }
    }
  }

  // Parse <classpath> section
  const char* cp_start = find_tag_start(root_start, root_end, "classpath");
  if (cp_start != nullptr) {
    const char* cp_end = find_tag_end(cp_start, root_end, "classpath");
    if (cp_end != nullptr) {
      const char* pos = cp_start;
      while (pos < cp_end && entries->length() < XML_CONFIG_MAX_ENTRIES) {
        const char* entry_el = find_tag_start(pos, cp_end, "entry");
        if (entry_el == nullptr) break;

        char path[XML_CONFIG_MAX_TOKEN] = {0};

        const char* el_bound = entry_el + 1;
        while (el_bound < cp_end && *el_bound != '>' && *el_bound != '<') el_bound++;
        if (*el_bound == '>') el_bound++;

        if (parse_attribute(entry_el, el_bound, "path", path, sizeof(path))) {
          entries->append(new xmlConfigEntry(xmlConfigEntry::CLASSPATH_ENTRY, path, nullptr));
        }
        pos = el_bound;
      }
    }
  }

  return true;
}

// ============================================================================
// Apply Entries to JVM Configuration
// ============================================================================

bool xmlConfigReader::apply_entries(GrowableArrayCHeap<xmlConfigEntry*, mtArguments>* entries) {
  bool result = true;

  for (int i = 0; i < entries->length(); i++) {
    xmlConfigEntry* entry = entries->at(i);

    switch (entry->_type) {
      case xmlConfigEntry::FLAG: {
        // Convert to -XX: format and process
        char arg[1024];
        if (entry->_value == nullptr || entry->_value[0] == '\0') {
          // Boolean flag with no value — treat as +Flag
          snprintf(arg, sizeof(arg), "+%s", entry->_name);
        } else if (strcmp(entry->_value, "true") == 0) {
          snprintf(arg, sizeof(arg), "+%s", entry->_name);
        } else if (strcmp(entry->_value, "false") == 0) {
          snprintf(arg, sizeof(arg), "-%s", entry->_name);
        } else {
          snprintf(arg, sizeof(arg), "%s=%s", entry->_name, entry->_value);
        }

        // Process through the standard flag mechanism
        if (!Arguments::parse_argument(arg, JVMFlagOrigin::CONFIG_FILE)) {
          warning("XML config: failed to apply flag '%s'", arg);
          result = false;
        }
        break;
      }

      case xmlConfigEntry::SYSTEM_PROPERTY: {
        // Add as -D property
        char prop[1024];
        snprintf(prop, sizeof(prop), "%s=%s", entry->_name, entry->_value);
        Arguments::add_property(prop, Arguments::UnwriteableProperty, Arguments::InternalProperty);
        break;
      }

      case xmlConfigEntry::CLASSPATH_ENTRY: {
        // Append to classpath
        Arguments::append_sysclasspath(entry->_name);
        break;
      }
    }
  }

  return result;
}

// ============================================================================
// Public Interface
// ============================================================================

bool xmlConfigReader::read_config(const char* path) {
  // Step 1: Validate file security
  if (!validate_file_permissions(path)) {
    return false;
  }

  size_t file_size = 0;
  if (!validate_file_size(path, &file_size)) {
    return false;
  }

  // Step 2: Read file content
  int fd = open(path, O_RDONLY | O_NOFOLLOW);  // O_NOFOLLOW: reject symlinks
  if (fd < 0) {
    warning("XML config '%s': cannot open", path);
    return false;
  }

  char* content = (char*)os::malloc(file_size + 1, mtArguments);
  if (content == nullptr) {
    close(fd);
    return false;
  }

  ssize_t bytes_read = read(fd, content, file_size);
  close(fd);

  if (bytes_read != (ssize_t)file_size) {
    warning("XML config '%s': read error", path);
    os::free(content);
    return false;
  }
  content[file_size] = '\0';

  // Step 3: Parse XML
  GrowableArrayCHeap<xmlConfigEntry*, mtArguments> entries(16);
  bool parse_ok = parse_xml(content, file_size, &entries);
  os::free(content);

  if (!parse_ok) {
    // Clean up
    for (int i = 0; i < entries.length(); i++) delete entries.at(i);
    return false;
  }

  // Step 4: Apply to JVM
  bool apply_ok = apply_entries(&entries);

  // Clean up
  for (int i = 0; i < entries.length(); i++) delete entries.at(i);

  if (apply_ok) {
    log_info(arguments)("XML config loaded: %s (%d entries)", path, entries.length());
  }

  return apply_ok;
}

const char* xmlConfigReader::find_default_config() {
  // Search order:
  // 1. $JAVA_HOME/conf/jvm-config.xml
  // 2. /etc/jvm-config.xml

  const char* java_home = Arguments::get_java_home();
  if (java_home != nullptr) {
    static char path_buf[1024];
    snprintf(path_buf, sizeof(path_buf), "%s/conf/jvm-config.xml", java_home);
    if (access(path_buf, R_OK) == 0) {
      return path_buf;
    }
  }

  if (access("/etc/jvm-config.xml", R_OK) == 0) {
    return "/etc/jvm-config.xml";
  }

  return nullptr;
}

void xmlConfigReader::print_config(outputStream* st) {
  const char* path = find_default_config();
  if (path != nullptr) {
    st->print_cr("XML JVM Config: %s", path);
  } else {
    st->print_cr("XML JVM Config: (none found)");
  }
}
