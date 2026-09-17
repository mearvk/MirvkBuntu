# file-systems

Repository-level copies of the custom filesystems developed for the Ubuntu
White Edition kernel, gathered here for convenient reference and review.

## Contents

```text
file-systems/
└── tac3/            TAC3 — n-tuple redundant File System
    ├── tac3.h       TAC1/TAC2/TAC3 structs, wear/pressure model, constants (C)
    ├── tac3.c       VFS filesystem + wear/pressure/health tables (kernel C)
    ├── tac3.hpp     C++ port of the portable engine (Tables 1/2/3)
    ├── tac3.cpp     C++ port implementation (userspace; simulation/testing)
    ├── Kconfig      config TAC3
    └── Makefile     obj-$(CONFIG_TAC3) += tac3.o  +  `make tac3-cpp`
```

## TAC3 in brief

TAC3 maintains three coordinated tables over a block device:

- **Table 1 — Primary File Table:** canonical per-file/extent metadata.
- **Table 2 — Redundancy / Integrity:** an N-way, per-region, per-layer
  wear/health mirror (default 10× redundancy) so damage is evident and
  recoverable.
- **Table 3 — Admin / State:** device/media/administrative facts only
  (tech id, monitor health, table health, permits, timestamps) plus opaque
  operator-defined reference slots the kernel assigns **no** meaning to. The
  kernel does not compute or judge any human attribute.

Read/write "touches" accumulate weighted wear per region; clustered or
jarring access compounds recorded impact, and pressure/quality are scaled
against the drive's "touch grace" (media class, rated speed, endurance).

## Authoritative source

These are **copies**. The build-wired, authoritative source lives in the
vendored kernel tree and is what actually compiles:

```text
kernels/linux-5.15.204/linux-5.15.204/fs/tac3/
```

wired into `fs/Kconfig` (`source "fs/tac3/Kconfig"`) and `fs/Makefile`
(`obj-$(CONFIG_TAC3) += tac3/`). Edit the kernel-tree copy for anything that
must build; keep this reference copy in sync when it changes.


## C++ port (`tac3.hpp` / `tac3.cpp`)

`tac3.hpp`/`tac3.cpp` are a **portable, standalone C++ equivalent** of the pure
TAC3 engine — the N-layer ("multitude") redundant file table, the per-region /
per-layer wear accounting, and the read **quality** / **pressure** / disk-health
derivations — exposed as a `tac3::Tac3Engine` class in namespace `tac3`.

They exist for simulation, testing, and tooling. The arithmetic matches the
kernel module exactly (same MB/s speed ceilings; `quality = min(1000,
observed*1000/ceiling)`; `pressure = +50 per doubling of read-heat`; a *jarring*
access spreads extra impact across all N layers; green/white/yellow health).
The mountable-filesystem VFS glue in `tac3.c` has no userspace equivalent and is
not part of the port.

Build the port with a host compiler (both Makefiles support it):

```sh
make tac3-cpp     # compiles tac3.cpp -> tac3_cpp.o
make cpp-clean    # removes the object
```

The C++ port and the C kernel module are kept as parallel expressions of the
same model; keep them in sync when the engine changes.


## ISO and installer integration

TAC3 is wired into the distribution in two complementary ways:

1. **ISO / kernel (the filesystem module).** `CONFIG_TAC3=m` is set in the
   kernel defconfig (`arch/x86/configs/galactic_cherry_defconfig`) and the
   checked-in `.config`, alongside `CONFIG_NEGAMANE=m`. The normal build flow
   (`make kernel-defconfig && make kernel-install && make iso`) compiles
   `tac3.ko`, `modules_install` copies it into the rootfs
   (`/lib/modules/<ver>/`), and `scripts/gen-iso.sh` folds the rootfs into
   `filesystem.squashfs` — so TAC3 ships on the ISO. No change to
   `gen-iso.sh` is needed; enabling the CONFIG symbol is sufficient.

2. **Binary installer (the userspace tool).** `tac3ctl` (`tools/tac3/`, built
   from the C++ port) is registered in `installer/install-manifest.txt`:

   ```text
   tac3ctl|tools/tac3|tac3ctl|1
   ```

   The `package-installer` binary reads that manifest at runtime, so `tac3ctl`
   is installed into `/user/bin` and `/deck/bin` **without recompiling the
   installer**. Adding future technology is likewise a one-line manifest change.
