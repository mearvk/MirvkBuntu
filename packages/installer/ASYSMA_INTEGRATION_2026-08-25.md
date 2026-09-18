# Installer ASYSMA Integration — 2026-08-25

The installer now has a documented integration target for today's ASYSMA and SecureJDK 28 work.

## Installation contract

The installer may install and register the following components when selected:

```text
SecureJDK 28
ASYSMA runtime/launcher
JDesk ASYSMA application
ASYSMA icon assets
Desktop/application launcher metadata
```

The existing installer remains the platform installation authority. ASYSMA is an installed application/runtime format and does not replace the operating-system package or executable loader.

## Runtime relationship

```text
Installer
   |
   +--> SecureJDK 28
   |
   +--> ASYSMA runtime
   |       |
   |       +--> native bootstrap
   |       +--> host/integrity policy
   |       +--> SecureJDK 28 bridge
   |
   +--> JDesk.asysma
   |
   +--> CMD-origin icon family
```

## JDesk first target

The first concrete ASYSMA desktop target is JDesk. Its intended entry mode is:

```text
NATIVE_THEN_JAVA
```

The native portion performs only the required bootstrap, host profiling, capability selection, integrity/policy checks, and documented handoff. The Java portion launches the JDesk application through SecureJDK 28.

## Architecture

Initial native target:

```text
x86-64 Intel/AMD
```

Platform-native representations remain:

```text
Linux   -> ELF
Windows -> PE/COFF
macOS   -> Mach-O
```

The installer must not label x86-64-v3 binaries as universal x86-64 binaries. CPU capability selection belongs to the ASYSMA runtime.

## Optional installation behavior

The installer should present ASYSMA integration as an explicit component/feature. A normal JDK installation must remain functional without ASYSMA.

Suggested components:

```text
[ ] SecureJDK 28
[ ] ASYSMA runtime
[ ] JDesk desktop application
[ ] Desktop integration / icons
```

The implementation should preserve existing installation scopes and should not request elevated privileges unless the selected scope requires them.

## Integrity and failure behavior

Before a native ASYSMA payload is executed, the runtime must validate package structure, required architecture/capability, payload integrity, and policy. Unsupported or malformed packages fail closed.

The installer itself should verify the files it installs and should not silently execute an untrusted ASYSMA payload merely because it is present on disk.

## XMC relationship

No XMC compiler has been located in the repository branches examined as of this record. The installer therefore treats ASYSMA packaging as an independent packaging step for now. If XMC is subsequently identified, it can be integrated as the build-time producer of `.asysma` packages without changing the installation contract.

## Status

This document records the installer integration design for 2026-08-25. It is not a claim that every ASYSMA binary runtime component is already production-complete.
