/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mirvkbuntu_installer.h — constants and contract for the edition-aware
 * MirvkBuntu system installer ELF (mirvkbuntu-installer).
 *
 * Unlike white-installer (a control plane that only delegates to the Bash
 * engine) and package-installer (which copies package artifacts), this binary
 * is a full disk installer engine: it partitions, formats, copies the root
 * filesystem, applies EDITION-SPECIFIC configuration, and installs the
 * bootloader for the three MirvkBuntu ISO editions:
 *
 *     slim     — the full GNOME OS that normally loads to RAM (toram), here
 *                installed PERSISTENTLY to disk. The RAM-overlay / toram
 *                machinery is dropped and the Ubuntu White theme is applied.
 *     minimal  — the base MirvkBuntu system with no desktop by default
 *                (vanilla console/X target), smallest footprint.
 *     full     — the complete desktop edition (a.k.a. "desktop"): full copy,
 *                a chosen desktop (gnome | mate | vanilla) and the default
 *                optional component set.
 *
 * It honors the repository's White Edition safety contract:
 *   DISCOVER -> PLAN -> REVIEW -> CONFIRM -> ELEVATE -> EXECUTE -> VERIFY -> REPORT
 * and defaults to a non-destructive DRY RUN. Nothing is written to a disk
 * unless the run is explicitly authorized with --install/--confirm.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#ifndef MIRVKBUNTU_INSTALLER_H
#define MIRVKBUNTU_INSTALLER_H

#define MI_VERSION "1.0"
#define MI_PROGRAM "mirvkbuntu-installer"
#define MI_TITLE   "MirvkBuntu Edition Installer"

/* Editions this installer can lay down on disk. */
typedef enum {
    MI_EDITION_UNKNOWN = 0,
    MI_EDITION_SLIM,
    MI_EDITION_MINIMAL,
    MI_EDITION_FULL
} mi_edition_t;

/* Desktop environments (full/slim editions). Mirrors the Bash engine's set. */
typedef enum {
    MI_DESKTOP_DEFAULT = 0, /* resolved from the edition's own default */
    MI_DESKTOP_GNOME,
    MI_DESKTOP_MATE,
    MI_DESKTOP_VANILLA
} mi_desktop_t;

/*
 * Optional component contract. These mirror the Bash engine
 * (scripts/galactic-cherry-installer) COMPONENT_IDS/DESCS/DEFAULTS and the
 * white-installer orchestrator, so a delegated install stays consistent.
 *   ubuntu-white  ON   (forced ON for slim, which is the White edition)
 *   security      ON
 *   git-improved  ON
 *   jwstf         OFF
 */
#define MI_COMPONENT_COUNT 4
#define MI_DEFAULT_COMPONENTS "ubuntu-white security git-improved"

/* Edition markers written by the live build (build/build-slim.sh &c.). */
#define MI_EDITION_MARKER_DIR "/etc/mirvkbuntu"

/* Filesystem/label defaults, aligned with the build scripts' ISO labels. */
#define MI_ROOTFS_LABEL "MirvkBuntu"
#define MI_TARGET_MNT   "/mnt/mirvkbuntu-install"

/* Minimum target disk size (GiB) accepted for an install target. */
#define MI_MIN_DISK_GIB 8

/* Default RAM-overlay size carried over from the slim live image (informational). */
#define MI_SLIM_OVERLAY_DEFAULT "400M"

#endif /* MIRVKBUNTU_INSTALLER_H */
