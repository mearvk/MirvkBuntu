# Ubuntu Determinant Custom GNOME

This directory defines the upstream GNOME foundation and the customization boundary for the Ubuntu Determinant desktop.

## Upstream components

The production desktop is based on selected upstream GNOME components rather than treating MATE as the desktop implementation:

- GNOME Shell — desktop shell and core user interface.
- Mutter — compositor, window management, input/focus/workspaces, and display integration.
- Nautilus — GNOME Files file manager and filesystem UI.

Upstream source is maintained by the GNOME Project. This repository keeps the integration and customization work under `main/gnome/` while preserving upstream copyright and licensing notices.

## Ubuntu White Edition boundary

The visual identity is implemented as an additive overlay instead of a fork of GNOME source:

```text
X11 / Wayland display stack
        |
     Mutter
        |
   GNOME Shell
        |
 Ubuntu White Edition theme
        |
 Ubuntu White icon theme
        |
    Nautilus / Files
```

The approved desktop icon reference artwork is `images/desktop-icons/set-002/`. The ISO installer retrieves that exact set and installs it into the `Ubuntu-White` GNOME icon theme. The source artwork remains unchanged.

The initial visual layer includes GTK CSS, GNOME Shell CSS, GSettings defaults, and a GNOME-compatible icon theme. Behavioral changes should use GNOME extensions where practical; source patches are reserved for behavior that cannot be implemented cleanly at the extension/configuration layer.

## Source preservation policy

Do not overwrite upstream Ubuntu/GNOME source merely to customize appearance. Add Determinant changes as clearly identifiable overlay files, configuration, extensions, or patches. Preserve upstream COPYING/LICENSE files, SPDX notices, and third-party attribution for any imported source.

## Build choices

GNOME is the default production desktop:

```sh
make desktop
DESKTOP=gnome make desktop
```

The Ubuntu White Edition visual layer is enabled by default. It can be controlled with `GNOME_THEME`, `UBUNTU_WHITE_ICONS`, and `UBUNTU_WHITE_CSS`; see `BUILD_OPTIONS.md`.

MATE remains available as an independent option:

```sh
DESKTOP=mate make desktop
```

The MATE compatibility path remains pinned to its prior implementation and is not modified by the GNOME/Ubuntu White overlay.
