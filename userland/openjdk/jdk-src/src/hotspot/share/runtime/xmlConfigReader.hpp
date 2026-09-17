/*
 * Copyright (C) 2026 MEARVK LLC
 * SPDX-License-Identifier: GPL-2.0 WITH Classpath-exception-2.0
 *
 * xmlConfigReader.hpp - Secure XML Configuration Reader for JVM Start Flags
 *
 * Reads JVM startup flags from a signed/validated XML document.
 * Security features:
 *   - File permission validation (must be owned by root or launching user)
 *   - File must not be world-writable
 *   - Optional SHA-256 signature verification
 *   - XML parsing with strict bounds (no external entities, no DTD)
 *   - Maximum file size enforcement (64KB)
 *
 * XML Format:
 *   <?xml version="1.0" encoding="UTF-8"?>
 *   <jvm-config version="1" signature="sha256:...">
 *     <flags>
 *       <flag name="MaxHeapSize" value="4g"/>
 *       <flag name="UseG1GC" value="true"/>
 *       <flag name="GCTimeRatio" value="12"/>
 *     </flags>
 *     <system-properties>
 *       <property name="java.io.tmpdir" value="/var/tmp/jvm"/>
 *       <property name="file.encoding" value="UTF-8"/>
 *     </system-properties>
 *     <classpath>
 *       <entry path="/opt/app/lib/*"/>
 *       <entry path="/opt/app/conf"/>
 *     </classpath>
 *   </jvm-config>
 */

#ifndef SHARE_RUNTIME_XMLCONFIGREADER_HPP
#define SHARE_RUNTIME_XMLCONFIGREADER_HPP

#include "memory/allocation.hpp"
#include "utilities/growableArray.hpp"

// Maximum XML config file size (security: prevent resource exhaustion)
#define XML_CONFIG_MAX_SIZE (64 * 1024)

// Maximum number of flags/properties allowed in a single config
#define XML_CONFIG_MAX_ENTRIES 256

// Maximum length of a single flag name or value
#define XML_CONFIG_MAX_TOKEN 512

class xmlConfigEntry : public CHeapObj<mtArguments> {
public:
  enum EntryType {
    FLAG,             // -XX:Name=Value or -XX:+Name / -XX:-Name
    SYSTEM_PROPERTY,  // -Dname=value
    CLASSPATH_ENTRY   // added to -cp
  };

  EntryType   _type;
  char*       _name;
  char*       _value;

  xmlConfigEntry(EntryType type, const char* name, const char* value);
  ~xmlConfigEntry();
};

class xmlConfigReader : public AllStatic {
private:
  // Security validation
  static bool validate_file_permissions(const char* path);
  static bool validate_file_size(const char* path, size_t* out_size);
  static bool validate_signature(const char* content, size_t len, const char* expected_sig);

  // XML parsing (minimal, no external dependencies)
  static bool parse_xml(const char* content, size_t len,
                        GrowableArrayCHeap<xmlConfigEntry*, mtArguments>* entries);
  static const char* find_tag_start(const char* pos, const char* end, const char* tag_name);
  static const char* find_tag_end(const char* pos, const char* end, const char* tag_name);
  static bool parse_attribute(const char* element_start, const char* element_end,
                              const char* attr_name, char* out_value, size_t max_len);

  // Convert parsed entries to JVM arguments
  static bool apply_entries(GrowableArrayCHeap<xmlConfigEntry*, mtArguments>* entries);

public:
  // Main entry point - called from Arguments::parse_vm_init_args()
  // Returns true on success, false on failure (JVM will refuse to start)
  static bool read_config(const char* path);

  // Check if a config file exists at the standard location
  // Searches: $JAVA_HOME/conf/jvm-config.xml, /etc/jvm-config.xml
  static const char* find_default_config();

  // Diagnostic: print loaded configuration
  static void print_config(outputStream* st);
};

#endif // SHARE_RUNTIME_XMLCONFIGREADER_HPP
