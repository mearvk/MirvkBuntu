# MirvkBuntu Build System

The build system treats the **MirvkBuntu repository source authored by MEARVK LLC as authoritative**. `live-build` is only the ISO/filesystem assembly mechanism. It is not the project source and it must not silently substitute its own package or source definitions for MirvkBuntu.

## Source authority

Before a build starts, the scripts require these MirvkBuntu directories:

- `kernels/`
- `file-systems/`
- `sources/`
- `packages/`
- `userland/`
- `user-interface/`
- `gnome-source/`
- `docs/`

The package list is generated from **`packages/basic-packages.txt` in this repository** rather than being hard-coded as an Ubuntu package list.

The complete MirvkBuntu source set is staged into the image at:

```text
/opt/mirvkbuntu/source/
```

The build also writes:

```text
/etc/mirvkbuntu/source.conf
/etc/mirvkbuntu/source.sha256
```

The SHA-256 manifest records the exact author source files that entered the build staging area.

## Build authority order

```text
MirvkBuntu repository
        |
        | authoritative source + package manifest
        v
MirvkBuntu staging
        |
        v
live-build
        |
        | filesystem / boot / ISO assembly
        v
MirvkBuntu ISO
```

This deliberately separates **project authority** from **image assembly**. If a MirvkBuntu source addition is absent from the repository checkout, the build fails. If it is present, it is staged into the image and recorded in the source manifest.

`live-build` must not be treated as an alternate MirvkBuntu source tree.

## Development targets

- `build-minimal.sh`: minimum bootable development image.
- `build-desktop.sh`: desktop development image.
- `build-iso.sh`: convenience entry point for the desktop ISO.
- `prerequisites.sh`: installs the host-side ISO build prerequisites.

## Output

Build artifacts remain under:

```text
build/output/
```

The repository-level `output` path is a symlink to that directory:

```text
output -> build/output
```

A completed ISO is also copied to the user's desktop.

## Important limitation

The current build **stages the MirvkBuntu source into the resulting filesystem; it does not yet compile every kernel, GNOME component, or native source tree in `sources/` as part of the ISO build**. Those source-specific build pipelines should be integrated explicitly rather than allowing `live-build` to silently replace them.

That distinction is intentional: a missing MirvkBuntu-native build step must be added as a MirvkBuntu build step, not delegated implicitly to `live-build`.