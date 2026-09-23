# MirvkBuntu

Ubuntu for Mearvk Users in the United States.

MirvkBuntu is organized as an operating-system project rather than a single
application. Large upstream source trees are **downloaded on demand** by the
scripts in [`clone/`](clone/) rather than vendored into this repository, which
keeps the Git history practical while retaining reproducible, verifiable source
references.

## GNOME Desktop Themes

MirvkBuntu includes the Ubuntu White baseline plus four additional GNOME desktop theme profiles: **Royal**, **Archer**, **Creme**, and **Capability**. They share the same semantic color roles and Color Inferencer model while providing distinct visual palettes. See [`ubuntu-white/GNOME.THEMES.md`](ubuntu-white/GNOME.THEMES.md) for the theme specifications.

## Relative Aesthetic Design

MirvkBuntu uses a provisional **Relative Aesthetic** framework for desktop color and visual optimization. It assigns configurable 0–100 importance values to clarity, contrast, hierarchy, harmony, balance, semantic consistency, restraint, accessibility, and context stability before exact colors are generated. The optimizer then measures the resulting configuration against those priorities. See [`ubuntu-white/RELATIVE.AESTHETIC.VIEW.md`](ubuntu-white/RELATIVE.AESTHETIC.VIEW.md) for the design model and [`ubuntu-white/RELATIVE.AESTHETIC.FRAMEWORK.md`](ubuntu-white/RELATIVE.AESTHETIC.FRAMEWORK.md) for its mathematical definition.

## Repository Layout

```text
MirvkBuntu/
├── clone/                  # On-demand source reconciliation scripts (see below)
├── kernels/                # Linux kernel source trees (downloaded on demand)
├── gnome-source/           # GNOME desktop stack (cairo, glib, gtk, mutter, shell, …)
├── file-systems/           # Custom/experimental filesystems (e.g. tac3)
├── userland/               # Userland applications (chromium, darling, openjdk, …)
├── user-interface/         # User-interface components
├── ubuntu-white/           # "White edition" configuration
├── packages/               # Package manifests and installers
│   ├── aptitude/
│   ├── installer/
│   ├── basic-packages.txt
│   └── README.md
├── sources/                # Source working area (main, modules, libraries, scripts)
├── scripts/                # Helper scripts
├── synchro/                # Synchro: measured low-latency packet dispatch layer
├── build/                  # Build outputs and manifests
├── docs/                   # Documentation
├── model/                  # Model/reference material
├── international-criminal-court/
├── utf-4088/
├── pull.sh                 # Standalone GNOME-source fetch helper (see note below)
├── SOURCES.md              # Upstream source references
├── LICENSE
└── README.md
```

Directories are populated on demand; several are intentionally sparse until
their build responsibilities are defined.

## Acquiring the Sources

The primary entry point is the staged reconciler in `clone/`. From the
repository root:

```bash
bash clone/clone-all.sh
```

`clone-all.sh` runs eight stages in order and continues to the next stage even
if one fails (reporting a non-zero exit at the end if any stage failed):

| Stage | Script | Populates | Source |
|-------|--------|-----------|--------|
| 1 | `01-kernels.sh` | `kernels/` | kernel.org (kernel tarballs) |
| 2 | `02-file-systems.sh` | `file-systems/` | this repo |
| 3 | `03-sources.sh` | `sources/{main,modules,libraries,scripts}` | this repo |
| 4 | `04-packages.sh` | `packages/{aptitude,installer,securejdk-installer}` | this repo |
| 5 | `05-userland.sh` | `userland/` | `mearvk/Ubuntu.Determinant.Beta.Restricted` |
| 6 | `06-user-interface.sh` | `user-interface/` | this repo |
| 7 | `07-gnome-source.sh` | `gnome-source/` | this repo |
| 8 | `08-docs.sh` | `docs/` | this repo |

Stage 1 downloads the declared kernel source trees (5.15.204, 6.18.27, 6.19.14,
7.0.4) directly from kernel.org and extracts them under `kernels/`. Stages 2–8
reconcile a directory tree from a GitHub repository via the shared engine in
`clone/common-download.sh`.

You can also run any stage on its own, for example:

```bash
bash clone/05-userland.sh      # refresh only userland/
```

### How reconciliation works

`common-download.sh` enumerates the source subtree using the GitHub **Git Trees
API** (`git/trees/{sha}?recursive=1`), descending into child trees by SHA when a
response is truncated, then downloads each blob from
`raw.githubusercontent.com`. Every downloaded file is verified against its Git
blob SHA-1 (Git LFS content is exempt, since the tree SHA is the pointer's SHA).
Files already present with a matching size and SHA are skipped, so re-running a
stage only fetches what changed. A completed stage writes a `.clone-complete`
marker in its destination and is treated as authoritative on the next run.

### Authentication

Unauthenticated GitHub API access is limited to 60 requests/hour (per IP);
authenticated access raises this to 5000/hour, so authenticating is strongly
recommended for large reconciliations. `common-download.sh` discovers a token
in this order (first hit wins):

1. `GITHUB_TOKEN` or `GH_TOKEN` environment variable
2. `GITHUB_TOKEN_FILE` — a file containing just the token
3. `gh auth token` (GitHub CLI, if installed and logged in)
4. `~/.config/gh/hosts.yml` (the GitHub CLI hosts config)
5. an interactive Personal Access Token prompt, **only** when attached to a
   terminal (disable with `CLONE_NO_PROMPT=1`)

The simplest options are to run `gh auth login` once, or to export a token:

```bash
export GITHUB_TOKEN=ghp_your_token_here
bash clone/clone-all.sh
```

A classic PAT with `public_repo` (or `repo`) scope is sufficient for reading
these repositories.

### Configuration

The following environment variables adjust the reconciler's behavior:

| Variable | Default | Purpose |
|----------|---------|---------|
| `SOURCE_REPO` | `mearvk/MirvkBuntu` | Source repository for stages 2–4 and 6–8 |
| `SOURCE_REF` | `main` | Git ref to read from |
| `USERLAND_SOURCE_REPO` | `mearvk/Ubuntu.Determinant.Beta.Restricted` | Source repo for the userland stage |
| `USERLAND_SOURCE_REF` | `main` | Git ref for the userland stage |
| `GITHUB_TOKEN` / `GH_TOKEN` | *(unset)* | Authentication token |
| `GITHUB_TOKEN_FILE` | *(unset)* | Path to a file containing a token |
| `CLONE_NO_PROMPT` | `0` | Set to `1` to disable the interactive PAT prompt |
| `CLONE_RAW_DELAY` | `0` | Seconds to pause between raw downloads (eases raw-host throttling) |
| `CLONE_MAX_BACKOFF` | `900` | Maximum seconds to wait for a rate-limit reset before giving up |

> **Note:** `pull.sh` is a standalone helper that fetches only the
> `gnome-source/` tree from `Ubuntu.Determinant.Beta.Restricted` as a full-repo
> archive (via `curl` + `unzip`). It predates and overlaps stage 7
> (`07-gnome-source.sh`); prefer the `clone/` workflow for normal use.

## Building an Image

Two build paths are supported (both run on a Debian/Ubuntu host, as root, with
network access). See [`build/README.md`](build/README.md) for full details.

**Quick remaster (fast, no compilation)** — turn a stock Ubuntu ISO into a
MirvkBuntu variant with the repo's package set, `ubuntu-white/` theme, and
branding:

```bash
sudo bash build/prerequisites.sh remaster
sudo bash build/quick-remaster.sh /path/to/ubuntu-24.04-desktop-amd64.iso
```

**Native build (full distribution from source)** — compile the kernel (and,
where source is present, the GNOME stack and Chromium), then assemble the ISO
with live-build:

```bash
sudo bash build/prerequisites.sh native
bash gnome-source/pull-all-source.sh     # populate GNOME module source/
sudo bash build/bootstrap-native.sh
```

**Slim edition** — the full MirvkBuntu OS built from this repo's source,
delivered as a live image that loads entirely into RAM and boots to the full
GNOME desktop, with a 400 MB RAM-backed overlay for on-the-go work:

```bash
sudo bash build/prerequisites.sh native
bash gnome-source/pull-all-source.sh
sudo bash build/build-slim.sh            # -> build/output/MirvkBuntu-slim-amd64.iso
```

## Using `make`

A top-level `Makefile` wraps the build scripts with ordered, discoverable
targets (run `make help` for the full list). The scripts remain authoritative;
`make` is just convenience. Common flow:

```bash
sudo make prereqs      # install host build tooling (native path)
make sources           # fetch kernel + GNOME source (kernels + gnome-source)
sudo make slim         # build the slim ISO   (or: sudo make desktop / minimal)
```

Other targets: `make kernels`, `make gnome-source`, `make native`,
`make chromium`, `make bootstrap`, `make remaster ISO=/path/to/ubuntu.iso`,
`make clean`, `make distclean`. Build flags pass straight through
(see [`build/BUILD.FLAGS.md`](build/BUILD.FLAGS.md)), e.g.
`sudo make slim JOBS=8 BUILD_SKIP_CHROMIUM=1`. Module directories (`build/`,
`gnome-source/`, `kernels/`) each have their own `Makefile` too.

## Build Order

Both the main OS ISO and the Slim OS ISO are built from MirvkBuntu's own source.
Run the numbered scripts in the order shown, from the repository root, on a
Debian/Ubuntu host as root with network access.

### Main OS ISO (desktop)

| # | Command | Purpose |
|---|---------|---------|
| 1 | `bash build/prerequisites.sh native` | Install host build tooling (live-build, debootstrap, xorriso, toolchain). |
| 2 | `bash kernels/git.sh` | Fetch real Linux kernel source into `kernels/`. |
| 3 | `bash gnome-source/pull-all-source.sh` | Populate every GNOME module `source/` (pinned versions). |
| 4 | `bash build/build-desktop.sh` | Build the ISO → `build/output/MirvkBuntu-desktop-amd64.iso`. |

Step 4 (`build-desktop.sh`) internally runs, in order:

1. `require_commands live-build lb` — verify host tooling
2. `require_mirvkbuntu_source` — verify author source directories
3. `run_native_build` → `build/native-build.sh`, whose stages run in order:
   1. `build_kernels` (skip: `BUILD_SKIP_KERNELS=1`)
   2. `build_chromium` → `build/chromium/build-chromium.sh` (skip: `BUILD_SKIP_CHROMIUM=1`)
   3. `build_gnome` (skip: `BUILD_SKIP_GNOME=1`)
   4. `build_other_native_projects` (skip: `BUILD_SKIP_OTHER=1`)
   5. `verify_native_output` → `write_release_manifest` → `create_split_bundle`
4. `lb config` — configure live-build (desktop)
5. `write_package_list` → `stage_mirvkbuntu_source` → `write_mirvkbuntu_manifest` → `stage_native_outputs`
6. `run_live_build` (`lb build`) → `find_iso` → `publish_iso`

Steps 1–3 above can be run in one shot via `build/bootstrap-native.sh` (it
fetches the kernel then calls `build-desktop.sh`).

### Slim OS ISO (full OS to RAM, boots to GNOME)

| # | Command | Purpose |
|---|---------|---------|
| 1 | `bash build/prerequisites.sh native` | Install host build tooling. |
| 2 | `bash kernels/git.sh` | Fetch real Linux kernel source into `kernels/`. |
| 3 | `bash gnome-source/pull-all-source.sh` | Populate every GNOME module `source/` (pinned versions). |
| 4 | `bash build/build-slim.sh` | Build the ISO → `build/output/MirvkBuntu-slim-amd64.iso`. |

Step 4 (`build-slim.sh`) internally runs the same native gate as the desktop
build, then configures live-build for a RAM-loaded (`toram`) GNOME live session
with a 400 MB writable overlay:

1. `require_commands live-build lb`
2. `require_mirvkbuntu_source`
3. `run_native_build` → `build/native-build.sh` (same ordered stages as above)
4. `lb config` with `--bootappend-live "boot=live components toram ..."`
5. `write_package_list` (+ the slim GNOME package list) → `stage_mirvkbuntu_source` → `write_mirvkbuntu_manifest` → `stage_native_outputs`
6. slim live-session config (overlay-limit service, GDM autologin, GNOME session hook)
7. `run_live_build` (`lb build`) → `find_iso` → `publish_iso`

The only difference between the two ISOs is step 4's script
(`build-desktop.sh` vs `build-slim.sh`); steps 1–3 are identical.

## Synchro — measured low-latency packet dispatch

The [`synchro/`](synchro/) package is a self-contained networking layer for
timestamped packet dispatch and **measurement** of real per-destination latency.
It sends probes, matches replies, and reports the observed distribution
(min / p50 / p95 / p99, jitter, loss), plus an SLA-style report of the *measured*
fraction of destinations and samples that met a latency threshold.

- **`UdpDispatcher`** — timestamped UDP send/ack RTT measurement (`monotonic_ns`).
- **`LatencyStats`** — streaming per-destination percentiles, jitter, and loss.
- **`SlaReporter`** — measured `% of hosts/samples ≤ T ms` at a chosen percentile.
- **`@synchro` annotation + `load_backend`** — a networking backend that is
  imported and constructed lazily, on the first call (dynamic backend loading).
- **`MeteredHttp2Client` + `RateMeter`** — token-bucket-paced ("metered")
  HTTP/2 egress, suited to large or cross-region ("international") dataset
  transfers.

Synchro reports **measured** latency, not delivery-time guarantees: the
speed of light (~200 km/ms in fiber) and the best-effort nature of the Internet
make a fixed sub-10 ms guarantee to arbitrary destinations physically
impossible, so precision is expressed as an empirical, per-run figure. See
[`synchro/README.md`](synchro/README.md) for the API, the CLI
(`python -m synchro.cli`), and a measured loopback example.

## Relationship to Ubuntu.Determinant.Beta.Restricted

MirvkBuntu is the cleaner distribution-level home for the operating-system base.
The `userland/` tree is sourced from the maintained
`Ubuntu.Determinant.Beta.Restricted` reference repository (stage 5); the
remaining trees are reconciled from MirvkBuntu itself. Specialized research,
experimental filesystems, and application-specific work remain in their existing
projects until deliberately migrated.
