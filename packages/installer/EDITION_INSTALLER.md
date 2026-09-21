# MirvkBuntu Edition Installer (`mirvkbuntu-installer`)

**Project:** Ubuntu Determinant · MirvkBuntu
**Attention:** Max Rupplin — MEARVK LLC — 2026

`mirvkbuntu-installer` is the full, edition-aware system installer for the three
MirvkBuntu ISO editions. It is a single native C11 ELF built from
[`linux/mirvkbuntu_installer.c`](linux/mirvkbuntu_installer.c) (+
[`linux/mirvkbuntu_installer.h`](linux/mirvkbuntu_installer.h)).

## Why this exists

The repository already had two installer surfaces, but neither performed a real,
**edition-aware** install to disk:

| Existing tool | What it is | Gap |
|---|---|---|
| `scripts/galactic-cherry-installer` (Bash) | A disk installer that picks a **desktop** (mate/gnome/vanilla) and copies the running live filesystem. | Has **no concept of the slim / minimal / full ISO editions**. |
| `white-installer` (C) | A **control plane** that probes, previews, plans, and then *delegates* to the Bash engine. | Never installs anything itself. |

`mirvkbuntu-installer` closes that gap. It **owns the slim/minimal/full install
end to end**: partition → format → copy root filesystem → **edition-specific
configuration** → bootloader.

## The three editions

| Edition | Desktop default | Optional components | Edition-specific shaping |
|---|---|---|---|
| **slim** | GNOME | `ubuntu-white` (forced ON), `security`, `git-improved` | Converts the RAM/`toram` live image into a **persistent** on-disk install: purges `live-boot`/`live-config`, removes the RAM-overlay-limit service, applies the Ubuntu White theme. |
| **minimal** | vanilla (no desktop) | none by default | Smallest footprint. Boots to `multi-user.target` (console). |
| **full** (a.k.a. desktop) | GNOME | `ubuntu-white`, `security`, `git-improved` | Complete desktop edition; full copy + chosen desktop. |

The edition is **auto-detected** from the running live image when `--edition` is
omitted, using (in order): the `MIRVKBUNTU_EDITION` key in any
`/etc/mirvkbuntu/*.conf` marker, then an `edition=` token on the kernel command
line. An explicit `--edition` always wins.

## Safety contract

The installer follows the White Edition state model from
[`ARCHITECTURE.md`](ARCHITECTURE.md):

```
DISCOVER -> PLAN -> REVIEW -> CONFIRM -> ELEVATE -> EXECUTE -> VERIFY -> REPORT
```

- **The default run is a DRY RUN.** It discovers the host, resolves the edition,
  prints the full plan and an audit report, and exits **without touching any
  disk**.
- A destructive install requires **all** of: `--install` (or `--confirm`), a
  valid existing `--target` block device, `root`, and (unless `--yes`) an
  interactive confirmation of the data-loss warning.
- **No free-form shell.** Every value interpolated into the generated
  provisioning script is a typed, validated field. The target must match
  `/dev/...` with a restricted character set; username/hostname are
  ident-checked; full name and timezone reject shell-dangerous characters. A
  target such as `/dev/sda; rm -rf /` is refused.
- The user password is never written to disk or embedded in the script text: it
  is substituted into an in-memory copy immediately before execution and fed to
  `chpasswd` on standard input.

## Usage

```text
sudo mirvkbuntu-installer [OPTIONS]
```

| Option | Meaning |
|---|---|
| `--edition <slim\|minimal\|full>` | Edition to install (auto-detected if omitted). |
| `--desktop <gnome\|mate\|vanilla>` | Desktop environment (full/slim). |
| `--target`, `--disk <dev>` | Target block device, e.g. `/dev/sda`. |
| `--enable <comma,list>` | Turn optional components ON (layered on the edition defaults). |
| `--disable <comma,list>` | Turn optional components OFF. |
| `--hostname <name>` | System hostname. |
| `--username <name>` | Primary user login name. |
| `--fullname <name>` | Primary user full name. |
| `--timezone <Area/City>` | Timezone (default `America/New_York`). |
| `--install`, `--confirm` | Authorize the destructive install. |
| `--dry-run` | Plan only (this is the default). |
| `--non-interactive` | Do not prompt; use flags/defaults. |
| `--yes` | Assume yes at the final confirmation. |
| `--emit-script` | Print the generated provisioning script and exit. |
| `--help`, `-h` | Show usage. |

Optional components (mirror the Bash engine's contract):
`ubuntu-white` (ON), `security` (ON), `git-improved` (ON), `jwstf` (OFF).

### Examples

```sh
# Inspect exactly what a slim install would do — no writes:
mirvkbuntu-installer --edition slim --target /dev/sda --emit-script

# Dry-run plan for a minimal install:
mirvkbuntu-installer --edition minimal --non-interactive

# Real slim install to /dev/sda, headless:
sudo mirvkbuntu-installer --edition slim --target /dev/sda \
     --username max --hostname mirvk-slim --install --yes

# Full desktop, MATE, add the JWSTF web server:
sudo mirvkbuntu-installer --edition full --desktop mate --enable jwstf \
     --target /dev/nvme0n1 --install
```

## Build

Built by the native installer Makefile alongside the other binaries:

```sh
make -C packages/installer/linux mirvkbuntu-installer   # just this binary
make -C packages/installer/linux all                    # all native installers
```

A portable static build for the live ISO / minimal environments (needs a static
libc; may fail in a restricted sandbox — same caveat as `white-installer-static`):

```sh
make -C packages/installer/linux mirvkbuntu-installer-static
```

Install/uninstall the binary into the system tree:

```sh
make -C packages/installer/linux mirvkbuntu-installer-install     # -> /usr/bin
make -C packages/installer/linux mirvkbuntu-installer-uninstall
```

## How it ships on the ISO

`build/common-build.sh` gained a `stage_installer` step that the three ISO build
scripts call after staging the native outputs:

- `build/build-desktop.sh` → `stage_installer "$WORK_DIR" full`
- `build/build-slim.sh`    → `stage_installer "$WORK_DIR" slim`
- `build/build-minimal.sh` → `stage_installer "$WORK_DIR" minimal`

`stage_installer` compiles the installer (when a toolchain is present), copies
`mirvkbuntu-installer` (and `white-installer` and the `galactic-cherry-installer`
Bash engine) into `/usr/sbin` of the live root filesystem, writes an
`/etc/mirvkbuntu/edition.conf` marker so the edition auto-detects, and drops a
`/usr/bin/mirvkbuntu-install` wrapper that launches the installer with the ISO's
edition preselected:

```sh
# On the booted live system:
mirvkbuntu-install                 # dry-run plan for this ISO's edition
sudo mirvkbuntu-install --install --target /dev/sda
```

## Design rule

**Discover the edition. Plan it. Review it. Install explicitly. Keep the
executable the one authoritative artifact.**
