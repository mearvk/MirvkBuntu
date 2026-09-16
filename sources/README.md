# MirvkBuntu Source Set

MirvkBuntu keeps a small, auditable set of upstream source references rather than copying an entire Linux distribution into this repository.

## Basic source set

The initial source set follows the useful foundation already present in `Ubuntu.Determinant.Beta.Restricted`:

1. **Linux kernel** — Linux 5.15.204, matching the kernel source reference used by the existing project.
2. **GNOME core** — GLib, Cairo, GDK-Pixbuf, GTK, Mutter, and GNOME Shell.
3. **Basic packages** — a small Ubuntu/Debian base package manifest for a usable MirvkBuntu build environment.

The large upstream trees are acquired by the supplied scripts. They are not vendored into the Git history by default.

## Acquisition policy

- Prefer official upstream project locations.
- Record the exact version or revision in the local acquisition directory.
- Do not silently substitute a downstream fork for an upstream source.
- Keep MirvkBuntu-specific patches outside the pristine source checkout.
- Verify downloaded archives with SHA-256 when an upstream checksum is available.

## Layout

```text
sources/
├── README.md
├── kernel/
│   └── fetch-linux-kernel.sh
└── gnome/
    └── fetch-gnome-core.sh
```

After acquisition, the working trees are created under `sources/work/` (ignored by Git).
