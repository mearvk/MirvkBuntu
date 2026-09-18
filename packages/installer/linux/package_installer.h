/* SPDX-License-Identifier: GPL-2.0 */
/*
 * package_installer.h — constants for the White Edition package-installer ELF.
 *
 * package-installer installs the repository's package software either by
 * "disc" (a named bundle of components) or by "function" (all components whose
 * declared role/function matches a requested keyword). Unlike white-installer,
 * which is a control plane that delegates to the Bash engine, package-installer
 * installs DIRECTLY: it copies the resolved install artifacts into the target
 * location families. The destination trees are EDITION AWARE: on Ubuntu White
 * Edition it uses /deck, /user, and /system; on a standard Ubuntu it falls back
 * to the FHS locations /usr and /sbin.
 *
 * The component/source/name/default data is single-sourced from
 * installer/install-manifest.txt so this binary and the rest of the installer
 * stay in sync.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#ifndef PACKAGE_INSTALLER_H
#define PACKAGE_INSTALLER_H

#define PKG_VERSION "1.0"
#define PKG_PROGRAM "package-installer"
#define PKG_EDITION "Ubuntu Determinant — White Edition Package Installer"

/* Manifest describing the installable package software (component|source|name|default). */
#define PKG_INSTALL_MANIFEST "installer/install-manifest.txt"

/*
 * Direct-install destination roots — EDITION AWARE.
 *
 * Ubuntu White Edition uses its own install trees; on White Edition the package
 * software is installed into the /deck, /user, and /system trees. On a standard
 * (non-White) Ubuntu the FHS locations /usr and /sbin are used instead. The
 * edition is detected at runtime (see detect_edition()); it can be forced with
 * --edition white|standard or the PKG_EDITION environment variable.
 *
 * A destination is <root>/<subdir>: <root>/bin for ordinary tools, and on the
 * standard edition /sbin is used directly as a system-binary root.
 */

/* White Edition destination roots. */
#define PKG_WE_DECK_ROOT   "/deck"
#define PKG_WE_USER_ROOT   "/user"
#define PKG_WE_SYSTEM_ROOT "/system"

/* Standard (FHS) destination roots. */
#define PKG_STD_USR_ROOT   "/usr"    /* -> /usr/bin  */
#define PKG_STD_SBIN_ROOT  "/sbin"   /* used directly as a bin dir */

#define PKG_BIN_SUBDIR "bin"

/*
 * Optional explicit White Edition marker file. When present it forces White
 * Edition regardless of /etc/os-release. This lets a freshly-built rootfs
 * declare its edition before os-release branding is finalized.
 */
#define PKG_WE_MARKER "/etc/ubuntu-white-edition"
#define PKG_OS_RELEASE "/etc/os-release"

/* Max destination roots for any single edition. */
#define PKG_MAX_ROOTS 3

/* Upper bounds kept generous but fixed so the binary needs no heap growth. */
#define PKG_MAX_COMPONENTS 256
#define PKG_MAX_FIELD 256

#endif /* PACKAGE_INSTALLER_H */
