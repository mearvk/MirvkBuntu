# MirvkBuntu Source Imports

This document records source imported into MirvkBuntu from upstream/reference projects.

## Initial source import

The initial MirvkBuntu source set is intended to contain the core operating-system model, kernel source, GNOME source, and basic package source selected from the reference Ubuntu.Determinant.Beta.Restricted project.

Source is retained as source code rather than as prebuilt binaries. Upstream licenses and attribution files must remain with each imported component.

| Component | Reference | Destination |
|---|---|---|
| Operating-system model | Ubuntu.Determinant.Beta.Restricted | `model/` |
| Linux kernel source | Ubuntu.Determinant.Beta.Restricted / upstream Linux | `kernel/` |
| GNOME source | Ubuntu.Determinant.Beta.Restricted / upstream GNOME | `desktop/gnome/` |
| Basic packages | Ubuntu.Determinant.Beta.Restricted / upstream package sources | `packages/basic/` |

Exact upstream versions, commits, and licenses will be recorded alongside each imported source tree as the source is copied.

## Import rule

MirvkBuntu should not silently replace upstream source with generated binaries. Source provenance, version identity, license information, and MirvkBuntu modifications must remain inspectable.
