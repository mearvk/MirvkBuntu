# MirvkBuntu Install Build Method

> **Quick Limited release rule:** the ISO is created from the checked-out MirvkBuntu repository. No existing Ubuntu ISO is downloaded, extracted, remastered, or used as the source of the release image. Standard Ubuntu/Debian `.deb` package downloads through APT are permitted for the initial desktop experience.

## Purpose

This document describes the current MirvkBuntu method for turning a downloaded MirvkBuntu repository checkout into install media such as an ISO.

MirvkBuntu's own source tree, custom components, native compilation, dependency preparation, and build scripts remain authoritative. Canonical/Ubuntu and Debian tooling is used where appropriate for standard operating-system assembly and package acquisition.

## 1. Source Checkout

The process begins with a complete MirvkBuntu checkout.

The expected repository contains the major authoritative source areas:

- `kernels/`
- `file-systems/`
- `sources/`
- `packages/`
- `userland/`
- `user-interface/`
- `gnome-source/`
- `docs/`
- `build/`
- `installer/`
- `builder/`
- `scout/`

The build system verifies required source directories and the base package manifest before proceeding.

## 2. Custom Component Inventory

Before native compilation, MirvkBuntu reads:

`build/CUSTOM_COMPONENTS.txt`

The component registry identifies the custom software and source families that are intentionally part of MirvkBuntu.

The inventory tool:

`build/component-inventory.sh`

creates:

`build/work/component-inventory.txt`

The inventory records registered components, required source directories, additional top-level software/source directories, package manifests, and build scripts.

This provides a pre-compilation record of what the repository contains.

## 3. Dependency Scout

After component discovery, the native dependency scout prepares standard package dependencies.

The native program is:

`mirvkbuntu-scout`

Its source is under:

`scout/`

The scout reads:

`packages/basic-packages.txt`

and resolves package dependencies using APT metadata.

Resolved packages are recorded in:

`build/work/dependency-resolved.txt`

Downloaded Debian packages are stored in:

`build/cache/packages/`

The cache is intended to prevent unnecessary repeated downloads and to provide a known pre-build package storage location.

The scout can be built with:

    make scout

or:

    cmake -S scout -B build/scout -DCMAKE_BUILD_TYPE=Release
    cmake --build build/scout --config Release

A preflight check is available with:

    mirvkbuntu-scout --check

## 4. Native Compilation

Dependency preparation occurs before MirvkBuntu's native compilation stage.

The native build is handled by the existing MirvkBuntu build scripts rather than by live-build.

The build gate verifies that native compilation produces:

- a native root filesystem staging tree;
- native Debian packages;
- native release metadata;
- a native root filesystem SHA-256 manifest.

The resulting native artifacts are then staged for ISO assembly.

## 5. Package Staging

Native Debian packages are placed into the live-build package staging area:

`config/packages.chroot/`

The native root filesystem is staged into:

`config/includes.chroot/`

MirvkBuntu source and build metadata are also included in the resulting system so that the installed medium retains an identifiable record of its source origin and build inputs.

## 6. live-build

live-build is used for standard Linux live-system and ISO assembly.

It is not the authority for MirvkBuntu custom source.

Its role is to assemble the prepared MirvkBuntu artifacts into a bootable install/live medium while providing standard Debian/Ubuntu mechanisms such as:

- chroot/root filesystem assembly;
- APT package installation;
- package lists;
- bootable live-system construction;
- SquashFS creation;
- ISO generation.

The MirvkBuntu native source and compiled artifacts are prepared before this assembly stage.

## 7. ISO Generation

The current build targets use Ubuntu Noble and amd64 by default.

The common build configuration provides:

- Ubuntu suite: `noble`
- architecture: `amd64`
- ISO label: `MIRVKBUNTU`
- application identity: MirvkBuntu
- publisher identity: MEARVK LLC

The completed ISO is searched for in the live-build working directory.

It is then copied to:

`build/output/`

and to the user's Desktop output location.

The repository's `output` path is maintained as a symlink to the build output directory.

## 8. Native Builder

The cross-platform native builder is:

`mirvkbuntu-builder`

Its source is under:

`builder/`

It is designed to locate the MirvkBuntu repository and invoke the authoritative build pipeline.

Supported build selections currently include:

    mirvkbuntu-builder --desktop
    mirvkbuntu-builder --slim
    mirvkbuntu-builder --minimal
    mirvkbuntu-builder --all
    mirvkbuntu-builder --check

Parallel compilation can be requested with:

    mirvkbuntu-builder --desktop --jobs 8

The builder automatically invokes the dependency scout before beginning the ISO build.

## 9. Operating-System Support

### Linux

Linux uses the MirvkBuntu build scripts directly.

Required standard tools include the normal native compilation and ISO tooling, including Bash, CMake, CPack, live-build, xorriso, SquashFS tooling, debootstrap, and related utilities.

### Windows 10+

The native MirvkBuntu builder is compiled as a Windows executable.

ISO production uses WSL so that the Linux-native MirvkBuntu build and live-build environment can be used.

The dependency scout likewise uses WSL for APT operations.

### macOS

The native MirvkBuntu builder is compiled for macOS.

ISO production uses Docker or Podman to provide the Linux build environment required by live-build and the surrounding Linux toolchain.

The dependency scout uses the same container approach for APT dependency acquisition.

## 10. Current End-to-End Method

The complete current method is:

    Download or clone MirvkBuntu
              |
              v
    Locate authoritative source tree
              |
              v
    Validate required components
              |
              v
    Custom component inventory
              |
              v
    Dependency Scout
              |
              +----> APT metadata / Ubuntu-Debian .deb archives
              |
              +----> dependency resolution
              |
              +----> build/cache/packages/
              |
              v
    Native MirvkBuntu compilation
              |
              +----> native rootfs
              |
              +----> native .deb packages
              |
              +----> release metadata
              |
              +----> SHA-256 manifests
              |
              v
    Stage native artifacts
              |
              v
    live-build assembly
              |
              v
    Bootable ISO
              |
              v
    build/output/
              |
              v
    Desktop copy

## 11. Authority Model

MirvkBuntu uses a layered authority model.

### MirvkBuntu

Authoritative for:

- custom source;
- custom components;
- native compilation;
- MirvkBuntu package inputs;
- MirvkBuntu userland;
- MirvkBuntu user interface;
- custom kernel/source selections;
- custom installer and builder behavior.

### Dependency Scout

Responsible for:

- discovering package dependencies from the MirvkBuntu package manifest;
- resolving APT dependencies;
- acquiring Debian packages;
- maintaining the pre-build package cache;
- recording the resolved dependency set.

### live-build

Responsible for:

- standard live-system assembly;
- chroot package staging;
- filesystem/image construction;
- ISO creation.

live-build does not replace or override MirvkBuntu's authoritative custom source.

## 12. Build Artifacts

Important working locations are:

    build/cache/packages/
    build/work/component-inventory.txt
    build/work/dependency-resolved.txt
    build/work/dependency-cache.txt
    build/work/native/
    build/output/

These directories separate downloaded dependencies, inventory information, native compilation artifacts, and final install media.

## 13. Reproducibility and Verification

MirvkBuntu records source and native build information as part of the build process.

The native build produces SHA-256 information for the native root filesystem, while the live-build staging process preserves MirvkBuntu source metadata.

The component inventory and dependency manifest provide additional records of the inputs used before ISO assembly.

The intent is that an ISO can be traced back through:

    ISO
      -> live-build staging
      -> native artifacts
      -> dependency manifest/cache
      -> component inventory
      -> MirvkBuntu source checkout

## 14. Current Scope

This document describes the build method currently implemented in the repository.

The system is intentionally incremental. Additional custom software can be added to the component registry and package manifest before compilation. The Quick Limited path deliberately permits normal Ubuntu/Debian `.deb` acquisition for the first desktop experience while keeping the MirvkBuntu repository as the custom-source authority.

## 15. Short Method

In practical terms:

    1. Obtain MirvkBuntu source.
    2. Inventory its custom components.
    3. Scout and cache required Ubuntu/Debian package dependencies.
    4. Compile MirvkBuntu's native components.
    5. Verify native artifacts.
    6. Stage the compiled system.
    7. Use live-build for standard ISO assembly.
    8. Produce the MirvkBuntu install/live ISO.
    9. Validate the generated ISO and SHA-256 metadata.
    10. Store the ISO under build/output/ and publish a Desktop copy.

This is the current MirvkBuntu Install Build Method.
