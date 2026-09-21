# MirvkBuntu

Ubuntu for Mearvk Users in the United States.

MirvkBuntu is organized as an operating-system project rather than a single
application. Large upstream source trees are **downloaded on demand** by the
scripts in [`clone/`](clone/) rather than vendored into this repository, which
keeps the Git history practical while retaining reproducible, verifiable source
references.

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

## Relationship to Ubuntu.Determinant.Beta.Restricted

MirvkBuntu is the cleaner distribution-level home for the operating-system base.
The `userland/` tree is sourced from the maintained
`Ubuntu.Determinant.Beta.Restricted` reference repository (stage 5); the
remaining trees are reconciled from MirvkBuntu itself. Specialized research,
experimental filesystems, and application-specific work remain in their existing
projects until deliberately migrated.
