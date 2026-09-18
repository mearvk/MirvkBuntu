# GNOME Upstream Source Map

Ubuntu Determinant will track the following upstream GNOME components for its custom desktop implementation.

| Component | Upstream repository | Role |
|---|---|---|
| GNOME Shell | https://gitlab.gnome.org/GNOME/gnome-shell | Desktop shell, overview, application launching, panels, shell UI |
| Mutter | https://gitlab.gnome.org/GNOME/mutter | Display server/compositor, window management, focus, workspaces, input and monitor handling |
| Nautilus | https://gitlab.gnome.org/GNOME/nautilus | Files application and filesystem browsing/operations |

## Licensing

Each upstream component retains its own COPYING/LICENSE files and copyright notices. GNOME's primary licenses include GPL-2/GPL-3, LGPL-2, and CC BY-SA depending on the module and asset.

## Import strategy

The initial repository integration is intentionally documentation-first. The complete upstream source trees are large and should be imported as pinned upstream source snapshots or Git submodules, not silently flattened into this repository.

Before a source snapshot is accepted into the ISO build, record:

1. upstream repository URL;
2. exact release/tag or commit;
3. source archive checksum when an archive is used;
4. license/COPYING files;
5. Ubuntu Determinant patches;
6. build dependencies;
7. whether the component is enabled in the production ISO.

This makes the custom GNOME implementation reproducible and keeps upstream provenance intact.
