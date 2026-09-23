# MirvkBuntu Build Health

## Purpose

`BUILD.HEALTH.md` is the build-readiness contract for MirvkBuntu. The project is a custom source build: the MirvkBuntu repository is authoritative, native components are compiled from repository source, Debian/Ubuntu packages are dependency inputs, and `live-build` is used only for filesystem/ISO assembly.

The three supported product profiles use one pipeline:

**source checkout → source/dependency validation → native compilation → staging → profile selection → live filesystem → ISO → validation**

## Build profiles

| Area | Slim | Minimum | Full |
|---|---:|---:|---:|
| Custom MirvkBuntu kernel | ✓ | ✓ | ✓ |
| Core userland | ✓ | ✓ | ✓ |
| Base filesystem | ✓ | ✓ | ✓ |
| Package dependency resolution | ✓ | ✓ | ✓ |
| Bootable ISO | ✓ | ✓ | ✓ |
| Installer | — | ✓ | ✓ |
| Basic graphical environment | — | ✓ | ✓ |
| GNOME / desktop shell | ✓ | optional | ✓ |
| MirvkBuntu UI/theme | ✓ | ✓ | ✓ |
| Networking | ✓ | ✓ | ✓ |
| Audio | — | ✓ | ✓ |
| Printing/Bluetooth | — | optional | ✓ |
| Browser | — | optional | ✓ |
| Developer/build tools | — | — | ✓ |
| Documentation | ✓ | ✓ | ✓ |
| Reproducible build manifest | ✓ | ✓ | ✓ |
| ISO checksum/signing metadata | ✓ | ✓ | ✓ |

### Profile meaning

- **Slim**: full graphical MirvkBuntu live environment intended for RAM-backed operation and GNOME.
- **Minimum**: small bootable MirvkBuntu base intended to provide the minimum useful installed/live system while retaining the custom kernel and core userland.
- **Full**: the complete desktop product. The existing `desktop` build is the Full profile.

All three are selections from the same source/build pipeline. They are not separate distributions and do not use an existing Ubuntu ISO as an input.

## Required build pipeline

1. Clone or update `mearvk/MirvkBuntu`.
2. Verify authoritative source directories.
3. Inventory custom components.
4. Resolve Debian/Ubuntu package dependencies.
5. Compile MirvkBuntu-native components.
6. Record source, dependency, component, and build metadata.
7. Stage compiled artifacts into the target root filesystem.
8. Select Slim, Minimum, or Full.
9. Assemble filesystem and boot media with `live-build`.
10. Validate the generated ISO.
11. Produce SHA-256 and release metadata.
12. Publish artifacts under `build/output/`.

## Source authority

The build must never require an existing Ubuntu ISO. Ubuntu/Debian APT metadata, `.deb` dependencies, standard ISO tooling, and `live-build` assembly are allowed. Existing Ubuntu ISOs and extracted ISO filesystems are not source inputs for Slim/Minimum/Full.

The legacy remaster utility may remain available separately, but it is not part of the custom build path.

## Build state

Components should progress through:

```text
NOT_STARTED
DOWNLOADED
VERIFIED
CONFIGURED
COMPILED
TESTED
STAGED
INSTALLED
```

The health/preflight stage must fail rather than silently substitute a distribution binary when an authoritative MirvkBuntu component is required.

## Artifact layout

The canonical artifact directory is `build/output/`. Existing ISO names remain:

```text
MirvkBuntu-slim-amd64.iso
MirvkBuntu-minimal-amd64.iso
MirvkBuntu-desktop-amd64.iso
```

Every completed release should have an ISO, `.sha256`, `.release`, component/source metadata when available, and resolved package metadata when available.

## Acceptance checks

A profile is build-ready only when required source directories exist; component inventory succeeds; package dependency resolution succeeds; required native compilation succeeds; native artifacts are staged; the profile definition exists; `live-build` completes; the resulting file is a bootable ISO; the source SHA-256 manifest is present; the ISO SHA-256 sidecar matches; the release manifest exists; and no existing ISO was consumed as source.

## Commands

```bash
make health
make slim
make minimal
make full
make all-profiles
```

`make full` is an explicit alias for the existing desktop build.

Fresh-clone test:

```bash
git clone https://github.com/mearvk/MirvkBuntu.git
cd MirvkBuntu
make health
make prereqs
make sources
make slim
make minimal
make full
```
