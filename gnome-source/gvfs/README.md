# GVfs Source

This directory tracks the upstream **GVfs** source used by the Ubuntu Determinant GNOME desktop.

GVfs provides the virtual filesystem services and backends used through GIO for local and remote locations, mounts, trash, network resources, and other desktop filesystem operations.

## Layout

```text
gvfs/
├── README.md
├── pull-source.sh
└── upstream/
```

`pull-source.sh` downloads a pinned official GNOME release archive into `upstream/`. The pristine source is kept separate from future Ubuntu Determinant patches and offsets.

## Source policy

- Use the official GNOME release archive rather than requiring Git credentials.
- Verify the published SHA-256 checksum before extraction.
- Keep pristine upstream source under `upstream/`.
- Apply Determinant changes through a separate patch/offset layer.
- Preserve upstream licensing, notices, and attribution.

The initial release is pinned to **GVfs 1.60.0**. The GNOME source index lists the 1.60 series, including its release archive and checksum. urlGVfs official source indexhttps://download.gnome.org/sources/gvfs/1.60/

The version can be overridden with `GVFS_VERSION` when deliberately moving the desktop to another upstream release.
