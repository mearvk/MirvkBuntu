# Ubuntu White Edition — set-002 Icon Mapping

`images/desktop-icons/set-002/` is the quality reference/master artwork for the desktop icon treatment. The ISO build does not edit those source files.

The additive installer currently maps:

| Reference | GNOME role |
|---|---|
| `icon-001.png` | `places/folder.png` |
| `icon-002.png` | `places/folder-open.png` |
| `icon-003.png` | `places/user-home.png` |
| `icon-004.png` | `places/network-server.png` |
| `icon-005.png` | `places/drive-harddisk.png` |
| `icon-001..012.png` | `apps/ubuntu-white-001..012.png` |

This is intentionally an initial mapping. The remaining artwork can be assigned to specific GNOME application, MIME, device, and status roles as the icon vocabulary is finalized.

The build retrieves the exact repository assets from the `main` branch. This keeps the source artwork separate from the installed theme and avoids modifying upstream GNOME assets.
