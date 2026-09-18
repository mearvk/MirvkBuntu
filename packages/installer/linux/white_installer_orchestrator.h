/* SPDX-License-Identifier: GPL-2.0 */
/*
 * white_installer_orchestrator.h — constants for the White Edition smooth
 * install orchestrator ELF (the "usual" front door).
 *
 * The orchestrator wraps and DELEGATES to the existing Bash install engine
 * scripts/galactic-cherry-installer. It never reimplements disk/system logic.
 * Component ids/descriptions/defaults are single-sourced here so that --help
 * and the interactive selection step stay in sync with the Bash engine's
 * COMPONENT_IDS / COMPONENT_DESCS / COMPONENT_DEFAULTS parallel arrays.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#ifndef WHITE_INSTALLER_ORCHESTRATOR_H
#define WHITE_INSTALLER_ORCHESTRATOR_H

/* Version string reported in --help and the audit report. */
#define WIO_VERSION "1.0"
#define WIO_PROGRAM "white-installer"
#define WIO_EDITION "Ubuntu Determinant — White Edition Smooth Installer"

/*
 * Relative path (under the located repository root) to the Bash install
 * engine this orchestrator drives, and its program name for messages.
 */
#define WIO_ENGINE_RELPATH "scripts/galactic-cherry-installer"
#define WIO_ENGINE_NAME "galactic-cherry-installer"

/*
 * The optional component contract. These MUST mirror the Bash engine's
 * COMPONENT_IDS / COMPONENT_DESCS / COMPONENT_DEFAULTS exactly.
 *   ubuntu-white  ON
 *   security      ON
 *   git-improved  ON
 *   jwstf         OFF
 * DEFAULT_COMPONENTS = "ubuntu-white security git-improved".
 */
#define WIO_COMPONENT_COUNT 4
#define WIO_DEFAULT_COMPONENTS "ubuntu-white security git-improved"

#endif /* WHITE_INSTALLER_ORCHESTRATOR_H */
