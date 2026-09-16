# MirvkBuntu Source Imports

This document records source and documentation imported into MirvkBuntu from upstream/reference projects.

## Initial source import

The initial MirvkBuntu source set is intended to contain the core operating-system model, kernel source, GNOME source, basic package source, and supporting project documentation selected from the reference `Ubuntu.Determinant.Beta.Restricted` project.

Source is retained as source code rather than as prebuilt binaries. Upstream licenses and attribution files must remain with each imported component.

| Component | Reference | Destination |
|---|---|---|
| Operating-system model | Ubuntu.Determinant.Beta.Restricted | `model/` |
| Linux kernel source | Ubuntu.Determinant.Beta.Restricted / upstream Linux | `kernel/` |
| GNOME source | Ubuntu.Determinant.Beta.Restricted / upstream GNOME | `desktop/gnome/` |
| Basic packages | Ubuntu.Determinant.Beta.Restricted / upstream package sources | `packages/basic/` |
| Foundational project records | Ubuntu.Determinant.Beta.Restricted `markdown/1.md`–`markdown/4.md` | `docs/1.md`–`docs/4.md` |

## Documents moved

The first documentation batch has been copied into `docs/` as source-preserving documents:

- `docs/1.md` — Provenance and Purpose
- `docs/2.md` — Architecture and Operating Model
- `docs/3.md` — Safety, Evidence, and Authority
- `docs/4.md` — Verification, Maintenance, and Preservation

These four documents were copied from the reference repository's `markdown/` directory without changing their substantive text.

## Import rule

MirvkBuntu should not silently replace upstream source with generated binaries. Source provenance, version identity, license information, and MirvkBuntu modifications must remain inspectable.

Additional documents will be moved in batches so each imported file can be verified against its reference path before the import is expanded to the larger kernel, desktop, and package source trees.
