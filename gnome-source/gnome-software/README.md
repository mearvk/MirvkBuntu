# GNOME Software Source

This directory tracks the upstream **GNOME Software** application used by the Ubuntu Determinant GNOME desktop.

GNOME Software is the graphical software-center application for browsing, installing, removing, and updating software. It is kept as an explicit source boundary so the upstream implementation remains identifiable while Ubuntu White Edition customization can be maintained separately.

## Layout

```text
gnome-software/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads an official GNOME release archive into `upstream/`. It does not replace Ubuntu packages or modify upstream source in place.

## Source policy

- Use an official GNOME release archive rather than an interactive Git credential flow.
- Verify the published SHA-256 checksum before extraction.
- Keep pristine upstream source under `upstream/`.
- Keep Ubuntu Determinant changes as separate patches/offsets.
- Preserve upstream licensing, copyright, and attribution.

The initial version is GNOME Software **50.3**, matching the GNOME 50 source series used by the current GNOME desktop integration. The version may be overridden deliberately through `GNOME_SOFTWARE_VERSION`.

## Ubuntu White Edition integration

Visual customization belongs outside the pristine source tree. The Ubuntu White Edition theme, icons, and related desktop policy should be applied through the surrounding build/theme layers unless a source-level change is specifically required.
