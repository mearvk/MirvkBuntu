# GNOME Binary Cache

This directory is reserved for the compiled GNOME runtime packages used while developing the custom Ubuntu Determinant GNOME implementation.

## Current target

Ubuntu Resolute 26.04 LTS, amd64.

The initial runtime set is:

- `gnome-shell_50.1-0ubuntu1_amd64.deb` — GNOME graphical shell.
- `mutter_50.1-0ubuntu2_amd64.deb` — GNOME window manager/compositor.
- `nautilus_50.0-0ubuntu2_amd64.deb` — GNOME file manager.

These are upstream Ubuntu-packaged binaries used as a development/runtime baseline, not the final customized Desktop. GNOME Shell depends on Mutter and a substantial GNOME/GTK/GLib runtime, so these packages are not a complete standalone desktop.

The authoritative package pages and checksums are recorded in `fetch-ubuntu-gnome-binaries.sh`.

## Why binaries are not committed here yet

The repository GitHub connector available to this workspace can create UTF-8 repository files, but cannot upload arbitrary binary bytes directly from the Ubuntu package mirrors. Therefore this commit adds a reproducible fetch-and-verify mechanism rather than pretending that the `.deb` files have already been uploaded.

Run the fetch script on an Internet-connected Ubuntu build host to populate this directory with the exact packages. Do not commit unrelated host-generated files.
