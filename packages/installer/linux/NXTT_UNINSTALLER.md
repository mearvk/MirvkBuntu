# nxtt — Ubuntu White Edition Uninstaller

`nxtt` is the planned native uninstall-review utility for Ubuntu White Edition.

## Design

The utility presents software in three relationship categories:

1. **Main** — core usability and dependency components.
2. **Installed** — software explicitly present on the system.
3. **Sibling** — related or optional companion software.

Uninstalling a component should explain relevant relationships and invite the user to review only the affected remainder. The tool must not manufacture dependencies or imply that market importance overrides user choice.

## Keyboard model

- **Arrow keys** — select or deselect an item.
- **Space** — proceed with the currently designated action.
- **Double Space** — acknowledge the stated risk and stop for a performance/review decision.
- **Ctrl+Enter** — show a detailed relationship/dependency breakdown.
- **Q** — stop and leave the next action to the user.

The terminal presentation uses orange text for the nxtt interface.

## Safety model

This initial implementation is a **preview only** and performs no removal. A production release should:

- identify dependencies before removal;
- distinguish required, recommended, optional, and unrelated software;
- preserve system-critical components;
- display the exact proposed changes;
- require explicit confirmation for destructive actions;
- provide a clear cancellation path;
- record the decision and outcome in an audit log.

## Logging

A local SQLite database is the default practical implementation. A MySQL-compatible backend can be supported for managed or multi-machine deployments. Logs should contain timestamps, software identifiers, relationship findings, selected actions, confirmations, and results without storing unnecessary personal information.

## Current executable

`nxtt-uninstaller.c` is the first non-destructive terminal preview. It establishes the interaction vocabulary while the package-discovery and dependency engine are developed separately.
