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

## Two build paths

There are two supported ways to produce a MirvkBuntu image. Both run on a
Debian/Ubuntu host, as root, with network access.

### Path A — quick ISO remaster (fast, no compilation)

Turns a stock Ubuntu desktop ISO into a MirvkBuntu variant by unpacking it,
installing the `packages/basic-packages.txt` set into the live filesystem,
applying the `ubuntu-white/` theme and branding, and repacking a bootable
hybrid ISO. No kernel or source compilation. This is the fastest route to a
bootable variant.

```bash
sudo bash build/prerequisites.sh remaster
sudo bash build/quick-remaster.sh /path/to/ubuntu-24.04-desktop-amd64.iso
# -> build/output/MirvkBuntu-remaster-amd64.iso
```

Useful variables: `SKIP_PACKAGES=1` (theme/branding only, no chroot apt),
`-o <path>` (output ISO location), `ISO_LABEL`, `THEME_DIR`.

### Path B — native build from source (full distribution)

Compiles MirvkBuntu's own kernel (and, when their source trees are present, the
GNOME stack and Chromium), then assembles the ISO with `live-build`. The native
compilation gate (`native-build.sh`) runs first and refuses to fall back to
distribution binaries.

```bash
sudo bash build/prerequisites.sh native
sudo bash build/bootstrap-native.sh          # fetches kernel source, then builds
```

`bootstrap-native.sh` fetches real kernel source via `kernels/git.sh` if none is
present (default `6.12.110`; override with `MIRVKBUNTU_KERNEL_VERSIONS`), then
runs `build-desktop.sh`.

While the GNOME and Chromium source trees are still being populated, individual
native stages can be skipped to produce a partial build. Skips are logged and
recorded in `/etc/mirvkbuntu/native-release.env` (`MIRVKBUNTU_NATIVE_RELEASE`
is `true` only when every stage ran):

```bash
sudo BUILD_SKIP_GNOME=1 BUILD_SKIP_CHROMIUM=1 bash build/bootstrap-native.sh
```

| Variable | Effect |
|----------|--------|
| `BUILD_SKIP_KERNELS=1` | skip kernel compilation |
| `BUILD_SKIP_CHROMIUM=1` | skip Chromium compilation |
| `BUILD_SKIP_GNOME=1` | skip the GNOME stack |
| `BUILD_SKIP_OTHER=1` | skip other native project wrappers |

### Slim edition — full OS to RAM, boots to GNOME (native path)

`build-slim.sh` produces the full MirvkBuntu OS (built from this repository's
own source via the native gate) as a **live image that loads entirely into RAM**
(`boot=live ... toram`) and **boots straight to the full GNOME desktop**. It
provides a RAM-backed writable overlay for on-the-go work, **capped at 400 MB**
by default, that resets on reboot.

```bash
sudo bash build/prerequisites.sh native
bash gnome-source/pull-all-source.sh        # populate GNOME module source/
sudo bash build/build-slim.sh
# -> build/output/MirvkBuntu-slim-amd64.iso
```

It is not a generic live-build spin: like `build-desktop.sh` it runs
`native-build.sh` first and feeds only the MirvkBuntu-native outputs into
live-build. Useful variables: `SLIM_OVERLAY_SIZE` (default `400M`),
`SLIM_LIVE_USER` (autologin user, default `mirvk`). While GNOME/Chromium source
is still being populated, use the `BUILD_SKIP_*` flags for a partial build.

## Scripts

- `prerequisites.sh`: installs host build tooling. Modes: `remaster`, `native`, `all`.
- `quick-remaster.sh`: path A — remaster a stock Ubuntu ISO (see above).
- `bootstrap-native.sh`: path B — fetch kernel source, then run the native build.
- `build-slim.sh`: slim edition — full OS to RAM, boots to GNOME (native path).
- `native-build.sh`: the native compilation gate (kernel/GNOME/Chromium).
- `build-minimal.sh`: minimum bootable development image (native path).
- `build-desktop.sh`: desktop development image (native path).
- `build-iso.sh`: convenience entry point for the desktop ISO.

## Populating GNOME source

The native path compiles the GNOME stack from `gnome-source/<module>/source/`.
`gnome-source/pull-all-source.sh` runs each module's `pull-source.sh`, normalizes
the layout so `source/` is the canonical build input, and verifies each required
module. Run it once (needs network) before a native/slim build:

```bash
bash gnome-source/pull-all-source.sh            # all desktop-required modules
bash gnome-source/pull-all-source.sh glib gtk   # a subset
```

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

## Current source-tree status (native path)

The native compilation gate compiles whatever source is present and refuses a
distribution fallback. As of now:

- **Kernel:** real source is fetched on demand by `kernels/git.sh` /
  `bootstrap-native.sh` (the trees committed in `kernels/` are directory-listing
  placeholders, not compilable source).
- **GNOME:** several modules under `gnome-source/` still lack a `source/`
  subtree; until they are populated, use `BUILD_SKIP_GNOME=1` for a partial
  build.
- **Chromium:** requires a full `chrome/` build root plus depot_tools; use
  `BUILD_SKIP_CHROMIUM=1` if it is not yet available.

That distinction is intentional: a missing MirvkBuntu-native build step must be
added as a MirvkBuntu build step, not delegated implicitly to `live-build`. The
quick remaster path (A) exists precisely so a bootable variant can be produced
today without waiting on every native source tree.