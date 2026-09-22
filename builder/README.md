# MirvkBuntu Builder

**Binary name:** mirvkbuntu-builder

A small native program that users run after downloading or cloning MirvkBuntu. It locates the repository, selects an ISO edition, and invokes the repository's authoritative build pipeline. The builder is repository-source-driven: it does not download or remaster an existing Ubuntu ISO.

## Basic use

    ./mirvkbuntu-builder --check
    ./mirvkbuntu-builder --limited
    ./mirvkbuntu-builder --desktop
    ./mirvkbuntu-builder --slim
    ./mirvkbuntu-builder --minimal
    ./mirvkbuntu-builder --all
    ./mirvkbuntu-builder --desktop --jobs 8

Output media is written by the existing MirvkBuntu pipeline under build/output/.

## Platform model

- Linux: runs the native MirvkBuntu build scripts directly.
- Windows 10+: invokes the same Linux build scripts through WSL.
- macOS: invokes the same Linux build scripts inside a privileged Linux container using Docker or Podman.
- MirvkBuntu source remains authoritative.
- live-build remains an ISO assembly mechanism; the native MirvkBuntu compilation stage runs first.
- `--limited` is the quick first-desktop path; it may acquire standard Ubuntu/Debian `.deb` packages through APT during assembly.
- No builder target uses `build/quick-remaster.sh` or requires an existing ISO.

## Build the builder

    cmake -S builder -B build/builder -DCMAKE_BUILD_TYPE=Release
    cmake --build build/builder --config Release

The executable is mirvkbuntu-builder, or mirvkbuntu-builder.exe on Windows.

## Design

This is a native launcher/orchestrator rather than a second build system. The existing repository build scripts remain the implementation of the OS build pipeline while the builder gives users one simple executable entry point.
