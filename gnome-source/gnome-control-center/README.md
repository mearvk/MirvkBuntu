# GNOME Control Center Source

This directory tracks the upstream **GNOME Settings / gnome-control-center** source used by the Ubuntu Determinant GNOME desktop.

GNOME describes gnome-control-center as the main interface for configuring aspects of the desktop. The project is maintained upstream in GNOME's GitLab repository, with a read-only GitHub mirror. citeturn0search2

## Layout

```text
gnome-control-center/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads a pinned official GNOME release archive into `upstream/`. The pristine source is kept separate from Determinant customizations so that upstream updates can be incorporated without overwriting our changes.

## Source policy

- Prefer official GNOME release archives rather than interactive Git credentials.
- Verify the downloaded archive with its published SHA-256 checksum.
- Keep pristine upstream source under `upstream/`.
- Put Ubuntu White Edition / Determinant modifications into separate patches or overlay files.
- Preserve upstream licensing, copyright, and attribution.

The GNOME source archive service currently publishes gnome-control-center releases through the 50 and 51 series. The repository's Ubuntu package archive also provides corresponding Ubuntu source packages. citeturn0search0turn0search9

## Build integration

The source acquired here is intended to provide the foundation for the ISO's GNOME Settings application. Theme and UI changes should normally be implemented through GNOME/GTK settings, theme assets, or a clearly isolated patch layer rather than rewriting unrelated upstream code.
