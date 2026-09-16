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

The first documentation batch was copied into `docs/` as source-preserving documents:

- `docs/1.md` — Provenance and Purpose
- `docs/2.md` — Architecture and Operating Model
- `docs/3.md` — Safety, Evidence, and Authority
- `docs/4.md` — Verification, Maintenance, and Preservation

The second documentation batch is now also present:

- `docs/BUILD_SOURCE_SAFETY.md` — GNOME Source Import and Build Safety; reference SHA-1 `17a586b0d8bcb3a678bce3dea5bf8ce16d8c6b6a`
- `docs/CLAMAV.md` — ClamAV Malware Detection and Source-Tree Screening; reference SHA-1 `02df091eea06ff9bab9a1a84f7767e72a5fe9610`
- `docs/CERTIFICATES.md` — preserved reference certificate document; reference SHA-1 `2db1a682f8b7b76ed6a98f8d2f5466fd87704d6e`
- `docs/CHANGESTYLE3.md` — Software Change, Authorship & Attribution Method; reference SHA-1 `cdd2f7ab8ed68c843c40e3c24573e0e33c2c2405`

These documents were copied from the reference repository's `markdown/` directory as source-preserving records. Certificate language is preserved as a project document and is not independently presented here as external certification or institutional endorsement.

## Held for later import

`markdown/BUILD.md` has been inspected but is not yet copied because the available repository response is truncated before the complete document ends. MirvkBuntu will not create a partial `BUILD.md`; the complete reference content should be retrieved before migration.

## Import rule

MirvkBuntu should not silently replace upstream source with generated binaries. Source provenance, version identity, license information, and MirvkBuntu modifications must remain inspectable.

Additional documents will be moved in batches so each imported file can be verified against its reference path before the import is expanded to the larger kernel, desktop, and package source trees.
