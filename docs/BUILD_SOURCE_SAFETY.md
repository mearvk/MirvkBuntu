# GNOME Source Import and Build Safety

**Max Rupplin — MEARVK LLC — 2026**

## Purpose

The GNOME source tree is treated as third-party source material. A successful network transfer is **not** by itself proof that the source is complete, authentic, safe to execute, or safe to install.

The source-pull scripts therefore use a temporary checkout, shallow history, single-branch selection, Git object verification, required-file checks, and only then copy the source into `/gnome-source/<component>`.

## Initial Crawl Integrity

Git's commit/object identifiers provide repository provenance. For an independent content check, the build system also creates `gnome-source/SHA256SUMS.source` using SHA-256 over the imported source files.

SHA-256 is used here as a **content-integrity digest**, not as a replacement for upstream provenance. When available, a signed upstream tag or verified commit should remain the preferred authenticity check. The SHA-256 manifest answers a different question: *did the bytes currently present in our source tree change after the initial verified crawl?*

A changed digest must cause investigation rather than automatic acceptance.

## Partial Download Protection

A source import must:

1. use a temporary directory;
2. fail closed on network/Git errors;
3. verify the Git object database with `git fsck`;
4. verify the expected branch/ref;
5. verify required build and documentation files;
6. never replace the existing vendored tree with an incomplete checkout;
7. exclude `.git` metadata from the vendored copy unless explicitly required;
8. record the resulting source digest after import.

A partial or interrupted download must therefore leave the previously installed source tree untouched.

## Executable Safety

Source obtained from upstream is **data until deliberately compiled and reviewed**. Pull scripts must never execute downloaded source, generated binaries, configure helpers, tests, hooks, or other upstream programs merely to validate a download.

Compilation should occur as an unprivileged build user. Generated executables must not be trusted merely because they were produced by the build system. Tests, generators, code-generation tools, and helper programs should be treated as executable supply-chain components and reviewed according to the project's threat model.

## Installation Safety

The build process must distinguish:

```text
source → compile → stage → inspect → package → install
```

from:

```text
source → compile → silently modify host system
```

GNOME components should first install into a project-controlled `DESTDIR` or staging prefix under `build/`. The build gate rejects arbitrary host destinations so that a typo cannot silently turn an OS build into a host-system installation.

Before final packaging/install, inspect the generated install manifest for:

- unexpected absolute paths;
- writes outside the intended staging root;
- unexpected setuid/setgid files;
- executable files in unusual locations;
- symlinks escaping the package root;
- startup services or hooks not explicitly requested;
- files replacing system libraries outside the package boundary.

`sudo` should not be used for compilation. Privilege, when eventually required for installation into a real target system, should be limited to the reviewed package-install step.

## Supply-Chain Principle

The project does not equate “downloaded from GitHub” with “trusted.” Trust is layered:

```text
upstream identity
      ↓
commit/tag provenance
      ↓
Git object validation
      ↓
source completeness
      ↓
SHA-256 content manifest
      ↓
controlled compilation
      ↓
staged install inspection
      ↓
final package / OS image
```

Each stage is a separate opportunity to detect corruption, substitution, unexpected content, or an unsafe installation decision.

## Build Rule

**Do not execute what you have not verified, and do not install what you have not staged and inspected.**

The source-pull scripts and `scripts/build-safety.sh` implement this principle for the current GNOME source import.
