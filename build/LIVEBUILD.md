# MirvkBuntu LIVE-BUILD Integration

## Purpose

`LIVEBUILD.md` defines the role of **live-build** in MirvkBuntu.

MirvkBuntu remains the authoritative project source. The live-build system is used to turn that source into a bootable Debian/Ubuntu-based filesystem and ISO while preserving the authored MirvkBuntu additions.

The intended relationship is:

```text
MirvkBuntu Author Source
        |
        | kernels/
        | file-systems/
        | sources/
        | packages/
        | userland/
        | user-interface/
        | gnome-source/
        | docs/
        v
MirvkBuntu Build Staging
        |
        | package manifest
        | source manifest
        | configuration
        v
Debian/Ubuntu + live-build
        |
        | bootstrap
        | package resolution
        | filesystem construction
        | boot configuration
        | squashfs / ISO assembly
        v
MirvkBuntu Live ISO
```

The key principle is:

> **MirvkBuntu supplies the project identity and authored additions; Debian/Ubuntu supplies the standard operating-system foundation; live-build assembles the resulting image.**

## 1. MirvkBuntu is the Original Source

The build begins with the MirvkBuntu repository itself.

The following directories are required author source:

- `kernels/`
- `file-systems/`
- `sources/`
- `packages/`
- `userland/`
- `user-interface/`
- `gnome-source/`
- `docs/`

The build system verifies that these directories exist before invoking live-build.

The package foundation is read from:

```text
packages/basic-packages.txt
```

The build therefore does not begin by selecting a generic Ubuntu desktop definition and treating that definition as MirvkBuntu. Instead, the MirvkBuntu source tree is established first and the standard distribution components are added around it.

## 2. What Debian/Ubuntu Provides

Debian/Ubuntu remains the standard distribution foundation.

That foundation provides the ordinary operating-system components required for a usable live system, including:

- Debian/Ubuntu package repositories.
- Base filesystem and userspace packages.
- Package dependency resolution.
- Standard system utilities.
- System initialization and service infrastructure.
- Kernel and boot support where selected by the MirvkBuntu build configuration.
- Standard networking and device support.
- Bootable live-media infrastructure.

These components are the **standard base**, not a replacement for MirvkBuntu source.

The build can therefore use a Debian/Ubuntu base while still carrying MirvkBuntu-authored source, configuration, documentation, userland, interface work, and later native components.

## 3. What live-build Provides

`live-build` is the image-construction mechanism.

Its responsibility is principally assembly:

1. Bootstrap the Debian/Ubuntu base.
2. Configure the target architecture and distribution suite.
3. Resolve and install the package set.
4. Populate the target filesystem.
5. Add MirvkBuntu-authoritative files to the filesystem.
6. Prepare the live boot environment.
7. Compress the filesystem.
8. Construct the bootable ISO image.

The important boundary is that **live-build is an assembler, not the author of MirvkBuntu**.

If a MirvkBuntu feature requires compilation, transformation, generation, or installation that live-build does not perform automatically, that work must be added explicitly to the MirvkBuntu build pipeline.

## 4. Source Staging

The current build stages the complete required MirvkBuntu source set under:

```text
/opt/mirvkbuntu/source/
```

The resulting image also receives:

```text
/etc/mirvkbuntu/source.conf
/etc/mirvkbuntu/source.sha256
```

The configuration identifies the source repository and records:

```text
MIRVKBUNTU_SOURCE_REPOSITORY=mearvk/MirvkBuntu
MIRVKBUNTU_SOURCE_ROOT=/opt/mirvkbuntu/source
MIRVKBUNTU_AUTHORITATIVE=true
```

The SHA-256 manifest provides a record of the source files that entered the build staging area.

## 5. Standard Foundation, MirvkBuntu Additions

| Layer | Responsibility |
|---|---|
| Debian/Ubuntu | Standard distribution foundation |
| live-build | Live filesystem and ISO construction |
| MirvkBuntu | Project source, additions, configuration, packages, interface, documentation and native development |
| Future MirvkBuntu stages | Compilation and installation of MirvkBuntu-native components |

## 6. Adding Further MirvkBuntu Touches

Further MirvkBuntu features should be introduced through explicit build stages.

```text
MirvkBuntu source
      |
      +--> source validation
      |
      +--> native compilation
      |
      +--> package creation
      |
      +--> configuration installation
      |
      +--> user-interface installation
      |
      +--> GNOME integration
      |
      +--> kernel integration
      |
      +--> documentation
      |
      v
live-build filesystem assembly
      |
      v
ISO
```

This is preferable to adding undocumented changes after the image has already been assembled.

Each new MirvkBuntu component should have a defined:

- Source location.
- Build or installation procedure.
- Dependency set.
- Destination path.
- Configuration requirements.
- Verification procedure.
- Reproducibility expectation.

## 7. Protecting Author Source

A package supplied by Debian/Ubuntu must not silently overwrite a MirvkBuntu-authored file when that file represents a deliberate project addition.

Where a collision is possible, the build should explicitly define which component owns the path.

The desired rule is:

```text
Standard distribution file
        |
        | unless deliberately replaced
        v
MirvkBuntu integration
```

A deliberate replacement should therefore be represented by a build step, package, configuration file, or other documented mechanism rather than occurring accidentally as a side effect of package installation.

## 8. Build-Time Verification

The build should verify the MirvkBuntu source before image assembly.

At minimum:

- Required source directories exist.
- `packages/basic-packages.txt` exists.
- Source files can be hashed.
- MirvkBuntu source is staged into the image.
- `source.conf` identifies MirvkBuntu as authoritative.
- `source.sha256` records the staged author source.
- The resulting ISO is placed under `build/output/`.
- The repository-level `output` path remains the symlink to `build/output/`.

Additional verification can be added as native MirvkBuntu build stages are introduced.

## 9. Current Boundary

The current implementation deliberately stops short of claiming that every MirvkBuntu native source tree is compiled into the ISO.

At present, the build:

- Uses the MirvkBuntu source tree as authoritative input.
- Uses the MirvkBuntu package manifest rather than a hard-coded generic package list.
- Stages the MirvkBuntu source into the resulting filesystem.
- Records the source identity and SHA-256 manifest.
- Uses live-build for Debian/Ubuntu bootstrap and ISO assembly.

The following remain candidates for explicit MirvkBuntu-native build integration:

- Native kernel builds.
- MirvkBuntu-specific filesystem components.
- Native C/C++ source builds.
- GNOME source builds and customizations.
- User-interface compilation and installation.
- Custom package generation.
- Additional system services and configuration.
- Further desktop and visual integration.

These should be added as MirvkBuntu build stages rather than assumed to be provided by live-build.

## 10. Design Principle

MirvkBuntu is not intended to be an unmodified Debian/Ubuntu live image with a different name.

The intended construction is:

```text
        STANDARD FOUNDATION
      Debian / Ubuntu system
               |
               v
        LIVE-BUILD ASSEMBLY
               |
               v
       MIRVKBUNTU SOURCE
    authored additions and code
               |
               v
       MIRVKBUNTU INTEGRATION
       further project touches
               |
               v
        MIRVKBUNTU LIVE ISO
```

The standard distribution provides the reliable foundation. MirvkBuntu provides the project-specific source and additions. live-build connects the two into a bootable live image.

## 11. Future Direction

As MirvkBuntu develops, the build should progressively move more responsibility into explicit MirvkBuntu-native stages.

The long-term objective is not to make live-build responsible for understanding MirvkBuntu source. The objective is to make the MirvkBuntu build system prepare the exact project state first, then use live-build only where it is useful for standard live-image construction.

That keeps the architecture clear:

```text
Author
  |
  v
MirvkBuntu Source
  |
  v
MirvkBuntu Build
  |
  v
Standard Debian/Ubuntu Foundation
  |
  v
live-build Assembly
  |
  v
Verified MirvkBuntu ISO
```

This document should be updated whenever the boundary between MirvkBuntu-native build work and standard live-build assembly changes.

## 12. Mandatory Native Compilation Gate

The build boundary has now been tightened: **native MirvkBuntu compilation occurs before live-build is allowed to assemble the ISO.** A live-build work tree containing `auto/`, `.build/`, `cache/`, `chroot/`, `config/`, or `local/` is only intermediate state. It is never the release product.

The required sequence is:

```text
MirvkBuntu source
        |
        v
Source validation
        |
        v
Native compilation gate
        |
        +--> Linux kernels
        |      -> Debian kernel packages
        |
        +--> Chromium/Chrome source
        |      -> release browser binary and runtime assets
        |
        +--> GNOME platform modules
        |      -> release libraries, services and desktop components
        |
        +--> remaining native MirvkBuntu projects
        |
        v
Native output verification + SHA-256 manifest
        |
        v
Native package/rootfs staging
        |
        v
live-build Debian/Ubuntu assembly
        |
        v
SquashFS + bootloader + ISO generation
        |
        v
MirvkBuntu-desktop-amd64.iso
```

### 12.1 Compilation is a hard prerequisite

`build/native-build.sh` is now a mandatory gate. The desktop and minimal ISO scripts invoke it before the live-build configuration is accepted as complete.

The gate requires, where the corresponding source is part of the MirvkBuntu tree:

- every compilable kernel source tree under `kernels/` to produce a kernel package;
- a real Chromium/Chrome source tree containing the Chromium `chrome/BUILD.gn` target to produce a release browser binary;
- the required GNOME source modules to compile and install into the native staging root;
- recognized native projects under `sources/`, `userland/`, and `user-interface/` to use their explicit build systems rather than silently being skipped.

If a required source tree is absent, incomplete, ambiguous, or not compilable, the build **fails**. It does not silently substitute a Debian/Ubuntu binary for the missing MirvkBuntu component.

### 12.2 Release-oriented compilation

The native build is intended to produce a releasable build where the upstream project permits it.

For Chromium, the release configuration uses `is_debug = false`, `symbol_level = 0`, `blink_symbol_level = 0`, and `is_official_build = false`. This deliberately means a release Chromium build rather than claiming an official Google Chrome build; official Chrome branding/build requirements are a separate upstream matter.

For GNOME modules, the common builder uses Meson `--buildtype=release` where Meson is the upstream build system. GNOME projects commonly use Meson, and GNOME/GTK documentation describes the corresponding configure/compile/install flow.

For Linux kernels, the native gate uses the kernel build system and `bindeb-pkg` so the resulting kernel can enter the live-build system as a Debian package. live-build documents custom kernels as requiring Debian packages and supports disabling its automatic kernel installation with `--linux-packages none`.

### 12.3 The ISO must contain our native outputs

The ISO scripts now configure live-build with:

```text
--linux-packages none
```

This prevents live-build from automatically selecting a distribution kernel. The native kernel packages produced by MirvkBuntu are staged through `config/packages.chroot/` instead.

Native compiled files are staged into `config/includes.chroot/` before `lb build`. live-build installs package content and then processes chroot includes, so the explicitly staged MirvkBuntu files can replace corresponding standard filesystem content where the project intentionally owns that path.

Custom Debian packages are also placed in `config/packages.chroot/`. This is the live-build mechanism for installing locally built `.deb` files into the live system.

The result is therefore not:

```text
Debian/Ubuntu binaries -> ISO
```

but:

```text
Debian/Ubuntu foundation
        +
MirvkBuntu-compiled kernel/packages
        +
MirvkBuntu-compiled Chromium
        +
MirvkBuntu-compiled GNOME components
        +
MirvkBuntu native userland/interface binaries
        |
        v
live-build assembly
        |
        v
MirvkBuntu ISO
```

### 12.4 Successful native binaries are preserved

Native output is retained under:

```text
build/work/native/
├── artifacts/
│   ├── packages/
│   ├── chunks/
│   ├── artifacts.sha256
│   ├── release.env
│   ├── rootfs.sha256
│   └── rootfs.sha256.digest
└── rootfs/
```

The build records SHA-256 values for the staged native filesystem and packages before ISO assembly. This gives the release process a verifiable handoff between compilation and image construction.

### 12.5 Splitting native binaries for GitHub transfer

`build/native-build.sh` also creates a compressed native artifact bundle and uses Linux `split` with a default 90 MiB chunk size.

The chunk directory contains:

```text
mirvkbuntu-native-amd64.part-0000
mirvkbuntu-native-amd64.part-0001
...
SHA256SUMS
```

The chunk size intentionally stays below GitHub’s 100 MB hard limit for ordinary repository objects. Large generated binaries should still be treated as release artifacts or Git LFS material rather than as normal source files when appropriate.

`build/reassemble-native.sh` verifies `SHA256SUMS`, concatenates the chunks in order, verifies the resulting compressed archive, and extracts the native artifact set. Reassembly is therefore deterministic and checksum-protected.

### 12.6 LFS is later, not the current source boundary

Large File Storage and a more complete long-term source/binary distribution system can be introduced later. The immediate requirement is simpler and stricter:

```text
compile first
verify second
preserve successful binaries
transfer/reassemble when necessary
assemble ISO last
```

The ISO builder must never treat a live-build work directory as the finished product, and it must never use the existence of a standard Debian/Ubuntu binary as proof that the corresponding MirvkBuntu-native component was successfully built.

## 13. Final ISO Release Gate

An ISO release is considered build-complete only when all of the following are true:

- Native compilation completed successfully.
- Required native binaries/packages passed verification.
- Native SHA-256 manifests were generated.
- Native outputs were staged into the live-build configuration.
- Automatic distribution-kernel selection was disabled for the MirvkBuntu build.
- live-build completed its filesystem and binary stages without error.
- A real `.iso` file exists in `build/output/`.
- The ISO was copied to the configured Desktop destination.
- The ISO can be identified as the requested MirvkBuntu profile and architecture.

**A live-build folder structure is not an acceptable substitute for the ISO.** The release artifact is the completed bootable ISO.

The implementation remains deliberately strict: if the current MirvkBuntu repository does not yet contain a complete Chromium source tree or compilable kernel source tree, the new gate will expose that gap by failing before ISO assembly. That is preferable to producing an ISO that silently contains the standard distribution component.
