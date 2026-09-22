# MirvkBuntu Dependency Scout

`mirvkbuntu-scout` is the native pre-build dependency scout for MirvkBuntu.

## Purpose

The scout runs before native compilation and ISO assembly. It reads the authoritative MirvkBuntu package manifest, resolves package dependencies through APT metadata, and downloads missing target-system Debian packages into `build/cache/packages/`.

It records the resolved package set in `build/work/dependency-resolved.txt` and the cache location/count in `build/work/dependency-cache.txt`.

## Platforms

- Linux: uses the host APT tooling directly.
- Windows 10+: the native Windows scout uses WSL for the Ubuntu/Debian APT operation.
- macOS: the native macOS scout uses Docker or Podman with an Ubuntu container for the APT operation.

The scout does not replace MirvkBuntu's native source compilation. It prepares package inputs for the ISO build.

## Build

    cmake -S scout -B build/scout -DCMAKE_BUILD_TYPE=Release
    cmake --build build/scout --config Release

Or:

    make scout

## Use

    mirvkbuntu-scout --check
    mirvkbuntu-scout
    mirvkbuntu-scout --download

The default operation performs dependency resolution and downloads packages into the pre-build package cache.

## Build order

    MirvkBuntu checkout
          |
          v
    Custom component inventory
          |
          v
    Dependency Scout
          |
          +--> APT metadata
          |
          +--> dependency resolution
          |
          +--> build/cache/packages/
          |
          v
    Native MirvkBuntu compilation
          |
          v
    live-build ISO assembly
          |
          v
    MirvkBuntu ISO

`live-build` remains the ISO/root-filesystem assembly mechanism; MirvkBuntu's source tree and component registry remain authoritative for custom software.
