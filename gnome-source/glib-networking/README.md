# GLib Networking Source

This directory tracks the upstream `glib-networking` source used by the Ubuntu Determinant GNOME desktop.

`glib-networking` provides GIO networking implementations and TLS/networking integration used by GLib/GIO applications. It is kept as a source acquisition boundary so the upstream source remains identifiable and future Determinant changes can be applied as separate offsets/patches.

## Layout

```text
glib-networking/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads a pinned upstream release archive into `upstream/`. It does not modify or replace Ubuntu/GNOME source elsewhere in the repository.

## Source policy

- Prefer an official GNOME release archive over interactive Git credentials.
- Verify the downloaded archive with SHA-256 before extraction.
- Keep pristine upstream source under `upstream/`.
- Put Determinant modifications in a separate patch/offset layer.
- Preserve upstream licensing and attribution.

The version and checksum are configurable in the script so the ISO build can be advanced deliberately rather than silently following an unpinned development branch.
