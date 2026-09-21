# MirvkBuntu Chromium Build

Authoritative release build of Chromium for MirvkBuntu, from the source tree at
`userland/chromium/chromium-src/`.

`build-chromium.sh` is the single definition of the Chromium build. Both the
native build gate (`build/native-build.sh`) and the thin wrapper at
`userland/chromium/build.sh` funnel through it, so there is exactly one release
build (no duplicate debug pass).

## What it does

1. Bootstraps `depot_tools` (gn / autoninja / gclient) if missing.
2. Runs `gclient sync` + `runhooks` so the checkout is buildable
   (skip with `SKIP_SYNC=1` if the tree is already synced).
3. Generates a release GN build and compiles the `chrome` target.
4. Installs into `DESTDIR`:
   - `/opt/mirvkbuntu/chromium/` — the runtime payload,
   - `<PREFIX>/bin/mirvkbuntu-chrome` — launcher (with `chromium` /
     `chromium-browser` aliases).

## Usage

Standalone (installs to the current system root):

```bash
sudo bash build/chromium/build-chromium.sh
```

Into a staging rootfs (how the native gate calls it):

```bash
DESTDIR=/path/to/rootfs JOBS=8 bash build/chromium/build-chromium.sh
```

## Environment

| Variable | Default | Purpose |
|----------|---------|---------|
| `DESTDIR` | *(empty)* | Install root (e.g. the live rootfs staging tree) |
| `PREFIX` | `/usr` | Install prefix for the launcher |
| `JOBS` / `BUILD_JOBS` | CPU count | Build parallelism |
| `CHROMIUM_SRC` | `userland/chromium/chromium-src` | Source root |
| `CHROMIUM_OUT` | `<src>/out/MirvkBuntuRelease` | GN output dir |
| `DEPOT_TOOLS` | `userland/chromium/depot_tools` | depot_tools location |
| `CHROMIUM_TAG` | *(unset)* | If source missing, fetch this tag first |
| `SKIP_SYNC` | `0` | Set `1` to skip `gclient sync`/`runhooks` |
| `TARGET_CPU` | *(unset)* | Cross-compile target (e.g. `arm64`) |

## Requirements

~16 GB RAM, ~100 GB disk, and a multi-hour build on typical hardware. `git` and
`python3` must be available; depot_tools supplies `gn`/`autoninja`.
