/*
 * Copyright (C) 2026 MEARVK LLC. All rights reserved.
 *
 * xmlClassWatcher.hpp — Directory Watcher for XML Class Files
 * Galactic Cherry Marvell Edition 98
 *
 * Monitors designated directories for .xclass files and loads them
 * into the Secure JVM on detection. Provides hot-reload capability
 * for XML class files placed into watched directories.
 *
 * Watched directories (defaults):
 *   /opt/jvm/xclasses/        — System-wide XML classes
 *   /etc/jvm/xclasses/        — Admin-deployed XML classes
 *   ~/.jvm/xclasses/          — Per-user XML classes
 *
 * The watcher uses inotify (Linux) to detect file creation and
 * modification events. Classes are loaded carefully through the
 * standard XMLClassReader → ClassFileParser pipeline with full
 * security validation.
 */

#ifndef SHARE_CLASSFILE_XMLCLASSWATCHER_HPP
#define SHARE_CLASSFILE_XMLCLASSWATCHER_HPP

#include "memory/allocation.hpp"
#include "runtime/os.hpp"

class ClassLoaderData;
class JavaThread;

// ═══════════════════════════════════════════════════════════════════════════════
// Watched Directory Entry
// ═══════════════════════════════════════════════════════════════════════════════

struct XClassWatchDir {
  const char* path;
  int         watch_fd;       // inotify watch descriptor (-1 if inactive)
  int         trust_grade;    // minimum trust grade for classes from this dir
  bool        active;

  XClassWatchDir() : path(nullptr), watch_fd(-1), trust_grade(0), active(false) {}
};

// ═══════════════════════════════════════════════════════════════════════════════
// XMLClassWatcher — Directory Listener
// ═══════════════════════════════════════════════════════════════════════════════

class XMLClassWatcher : public AllStatic {
 private:
  static const int MAX_WATCH_DIRS = 16;
  static XClassWatchDir _dirs[MAX_WATCH_DIRS];
  static int _dir_count;
  static int _inotify_fd;
  static volatile bool _running;
  static volatile bool _initialized;

  // Internal
  static bool setup_inotify();
  static bool add_watch(const char* path, int trust_grade);
  static void process_event(const char* dir_path, const char* filename, int trust_grade);
  static void load_xclass_file(const char* full_path, int trust_grade);
  static void scan_directory(const char* path, int trust_grade);

 public:
  // ─────────────────────────────────────────────────────────────────────────
  // Lifecycle
  // ─────────────────────────────────────────────────────────────────────────

  // Initialize the watcher (called during JVM startup)
  static void initialize();

  // Start the background watcher thread
  static void start();

  // Stop the watcher (called during JVM shutdown)
  static void stop();

  // ─────────────────────────────────────────────────────────────────────────
  // Configuration
  // ─────────────────────────────────────────────────────────────────────────

  // Register a directory to watch
  // trust_grade: minimum trust grade assigned to classes from this directory
  //   0 = unrestricted (user classes)
  //   3 = system level
  //   5 = genius level (kernel-adjacent)
  static bool watch_directory(const char* path, int trust_grade);

  // Remove a watched directory
  static bool unwatch_directory(const char* path);

  // ─────────────────────────────────────────────────────────────────────────
  // Status
  // ─────────────────────────────────────────────────────────────────────────

  static bool is_running()     { return _running; }
  static bool is_initialized() { return _initialized; }
  static int  watched_count()  { return _dir_count; }

  // Print status to stream (for /proc or diagnostics)
  static void print_status(outputStream* st);
};

#endif // SHARE_CLASSFILE_XMLCLASSWATCHER_HPP
