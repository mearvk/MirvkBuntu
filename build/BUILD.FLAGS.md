# BUILD.FLAGS.md — MirvkBuntu Build Flags

All environment variables and flags understood by the MirvkBuntu build scripts,
grouped by purpose. Every flag is optional; the **Default** column shows the
value used when the variable is unset. Set a flag by exporting it or prefixing
the command, e.g.:

```bash
sudo JOBS=8 BUILD_SKIP_CHROMIUM=1 bash build/build-slim.sh
```

---

## 1. Common (all native builds)

Consumed by `build/common-build.sh`, shared by `build-desktop.sh`,
`build-minimal.sh`, and `build-slim.sh`.

| Flag | Default | Purpose |
|------|---------|---------|
| `ARCH` | `amd64` | Target architecture. |
| `UBUNTU_SUITE` | `noble` | Ubuntu base suite passed to `lb config`. |
| `ISO_LABEL` | `MIRVKBUNTU` | ISO volume label prefix. |
| `BUILD_ROOT` | `build/work` | Working directory for live-build stages. |
| `OUTPUT_ROOT` | `build/output` | Where finished ISOs are written. |
| `DESKTOP_OUTPUT` | `$HOME/Desktop` | Extra copy location for the finished ISO. |

## 2. Native compilation gate

Consumed by `build/native-build.sh` (run before live-build by every native
build). Skips are logged and recorded in `/etc/mirvkbuntu/native-release.env`;
`MIRVKBUNTU_NATIVE_RELEASE` is `true` only when no stage was skipped.

| Flag | Default | Purpose |
|------|---------|---------|
| `BUILD_SKIP_KERNELS` | `0` | Set `1` to skip kernel compilation. |
| `BUILD_SKIP_CHROMIUM` | `0` | Set `1` to skip the Chromium build. |
| `BUILD_SKIP_GNOME` | `0` | Set `1` to skip the GNOME stack. |
| `BUILD_SKIP_OTHER` | `0` | Set `1` to skip other native project wrappers. |
| `JOBS` | CPU count | Build parallelism (`make`/`ninja`/`meson`). |
| `KERNEL_FIND_MAXDEPTH` | `8` | Max depth for kernel-source discovery under `kernels/`. |
| `KERNEL_FIND_TIMEOUT` | `120` | Watchdog (seconds) for kernel discovery; fails cleanly instead of hanging. |
| `BUILD_ROOT` | `build/work` | Working directory. |
| `NATIVE_ROOT` | `$BUILD_ROOT/native` | Native build root. |
| `ARTIFACT_ROOT` | `$NATIVE_ROOT/artifacts` | Native artifact output (.deb, manifests). |
| `ROOTFS_STAGE` | `$NATIVE_ROOT/rootfs` | Staging rootfs the ISO is assembled from. |
| `MIRVKBUNTU_CHUNK_SIZE` | `90M` | Split size for the native artifact bundle. |

> **Note:** `verify_native_output` still requires at least one executable and
> one `.deb`, so skipping *every* stage will fail verification. Skips are for
> partial builds while some source trees are still being populated.

## 3. Bootstrap (path B one-shot)

Consumed by `build/bootstrap-native.sh` (fetches kernel source, then runs
`build-desktop.sh`). Also honors the native-gate flags above.

| Flag | Default | Purpose |
|------|---------|---------|
| `MIRVKBUNTU_KERNEL_VERSIONS` | `6.12.110` | Kernel version(s) to fetch if none present. |
| `BUILD_SKIP_KERNELS` | `0` | Skip fetching + compiling the kernel. |

## 4. Slim edition

Consumed by `build/build-slim.sh` (in addition to the Common + Native flags).

| Flag | Default | Purpose |
|------|---------|---------|
| `SLIM_OVERLAY_SIZE` | `400M` | RAM-backed writable overlay cap for on-the-go work. |
| `SLIM_LIVE_USER` | `mirvk` | Autologin user for the live GNOME session. |
| `THEME_DIR` | `ubuntu-white` | Source of the MirvkBuntu-White theme assets. |

## 5. Quick remaster (path A)

Consumed by `build/quick-remaster.sh` (remaster a stock Ubuntu ISO; no
compilation).

| Flag | Default | Purpose |
|------|---------|---------|
| `SKIP_PACKAGES` | `0` | Set `1` for theme/branding only (no chroot apt step). |
| `PACKAGE_MANIFEST` | `packages/basic-packages.txt` | Package list to install. |
| `THEME_DIR` | `ubuntu-white` | Theme assets to apply. |
| `ISO_LABEL` | `MIRVKBUNTU` | ISO volume label. |
| `ISO_APPLICATION` | `MirvkBuntu` | ISO application id. |
| `ISO_PUBLISHER` | `MEARVK LLC` | ISO publisher string. |
| `BUILD_ROOT` | `build/work/remaster` | Working directory. |
| `OUTPUT_ROOT` | `build/output` | Output directory. |

Command-line: `-o <path>` / `--output <path>` sets the output ISO path;
positional argument is the source Ubuntu ISO.

## 6. Chromium build

Consumed by `build/chromium/build-chromium.sh` (and via `build_chromium` in the
native gate).

| Flag | Default | Purpose |
|------|---------|---------|
| `DESTDIR` | *(empty)* | Install root (empty = install to system root). |
| `PREFIX` | `/usr` | Install prefix for the launcher. |
| `JOBS` / `BUILD_JOBS` | CPU count | Build parallelism. |
| `CHROMIUM_SRC` | `userland/chromium/chromium-src` | Source root. |
| `CHROMIUM_OUT` | `<src>/out/MirvkBuntuRelease` | GN output directory. |
| `DEPOT_TOOLS` | `userland/chromium/depot_tools` | depot_tools location. |
| `CHROMIUM_TAG` | *(unset)* | If source missing, fetch this tag first. |
| `SKIP_SYNC` | `0` | Set `1` to skip `gclient sync`/`runhooks`. |
| `TARGET_CPU` | *(unset)* | Cross-compile target (e.g. `arm64`). |

## 7. GNOME source acquisition

Consumed by `gnome-source/pull-all-source.sh` and the per-module
`pull-source.sh` scripts.

| Flag | Default | Purpose |
|------|---------|---------|
| `FORCE` | `0` | Set `1` to re-fetch modules whose `source/` already exists. |
| `MODULES` | *(desktop set)* | Space-separated module list to fetch. |
| `GNOME_VERSIONS_FILE` | `gnome-source/GNOME_VERSIONS` | Version-pin file. |
| `GNOME_CLONE_DEPTH` | `20` | Git shallow depth; `0` = full clone (needed for commit SHAs). |
| `<MODULE>_REF` | *(pin file / `main`)* | Pin a module to a git tag/branch/commit, e.g. `GTK_REF=4.14.5`. |
| `<MODULE>_URL` | GNOME mirror | Override a module's source URL. |
| `<MODULE>_VERSION` | *(unset)* | For gvfs/orca: fetch an official release tarball instead of git. |

Pins live in [`gnome-source/GNOME_VERSIONS`](../gnome-source/GNOME_VERSIONS);
an explicit environment value overrides the file.

## 8. Prerequisites installer

`build/prerequisites.sh <mode>` — positional mode, not an env var.

| Mode | Installs |
|------|----------|
| `remaster` | Host tooling for the quick remaster path (squashfs-tools, xorriso, …). |
| `native` | Tooling for the native path (live-build, debootstrap, toolchain, …). |
| `all` (default) | Both sets. |

---

## Common recipes

```bash
# Full slim build, 8 jobs
sudo JOBS=8 bash build/build-slim.sh

# Slim build without Chromium (faster), 512M overlay
sudo BUILD_SKIP_CHROMIUM=1 SLIM_OVERLAY_SIZE=512M bash build/build-slim.sh

# Desktop build, skip GNOME + Chromium while their source is incomplete
sudo BUILD_SKIP_GNOME=1 BUILD_SKIP_CHROMIUM=1 bash build/bootstrap-native.sh

# Quick remaster, theme/branding only, custom output path
sudo SKIP_PACKAGES=1 bash build/quick-remaster.sh ubuntu.iso -o build/output/MirvkBuntu.iso

# Pin the whole GNOME stack to newer tags and re-fetch
FORCE=1 bash gnome-source/pull-all-source.sh

# Fetch a single kernel version only
bash kernels/git.sh 6.12.110
```
