# GNOME Terminal Source

This directory tracks the upstream `gnome-terminal` source used by the Ubuntu Determinant GNOME desktop.

GNOME Terminal is the terminal emulator application for the GNOME desktop. The source is kept separate from the Ubuntu White Edition customization layer so upstream code remains identifiable and replaceable.

## Layout

```text
gnome-terminal/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads a pinned official GNOME release archive into `upstream/`. It does not require a GitHub username or password and does not modify upstream source outside this directory.

## Source policy

- Use the official GNOME source archive.
- Verify the published SHA-256 checksum.
- Preserve pristine upstream source under `upstream/`.
- Keep Ubuntu Determinant modifications as separate patches/offsets.
- Preserve upstream licenses, copyright notices, and attribution.

The script defaults to GNOME Terminal **3.60.0**, an available release in the official GNOME source archive. The version can be changed explicitly with `GNOME_TERMINAL_VERSION`.
