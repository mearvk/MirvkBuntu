# Orca Source

This directory tracks the upstream GNOME Orca screen reader used by the Ubuntu Determinant GNOME desktop.

## Layout

```text
orca/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads a pinned official GNOME release archive into `upstream/`, verifies its published SHA-256 checksum, and preserves the pristine source for later Determinant integration work.

## Source policy

- Use official GNOME release archives rather than interactive Git credentials.
- Verify the downloaded archive before extraction.
- Keep pristine upstream source under `upstream/`.
- Keep Determinant changes as separate offsets or patches.
- Preserve upstream licensing and attribution.

Orca is the GNOME accessibility/screen-reader component. Changes to its behavior should be isolated from the pristine upstream tree so accessibility updates can be incorporated without losing the original source.
