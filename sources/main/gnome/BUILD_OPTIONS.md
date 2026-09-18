# Ubuntu White Edition Desktop Build Options

The production ISO keeps upstream Ubuntu/GNOME components intact and applies the Determinant customization as an additive overlay.

## Desktop selection

```sh
make desktop                  # GNOME + Ubuntu White Edition defaults
DESKTOP=gnome make desktop    # GNOME + Ubuntu White Edition
DESKTOP=mate make desktop     # retain the pinned MATE path
```

## GNOME visual selection

The GNOME installer accepts:

```sh
GNOME_THEME=ubuntu-white     # default
GNOME_THEME=stock            # upstream GNOME appearance
UBUNTU_WHITE_ICONS=1         # install images/desktop-icons/set-002 (default)
UBUNTU_WHITE_ICONS=0         # do not install the custom icon overlay
UBUNTU_WHITE_CSS=1           # install Ubuntu White GTK/Shell styling (default)
UBUNTU_WHITE_CSS=0           # leave upstream styling in place
```

MATE remains an independent desktop option. The Ubuntu White GNOME overlay is not forced onto MATE.

## Preservation policy

We do not overwrite upstream Ubuntu/GNOME source merely to change appearance. Custom behavior and visual changes are stored as files under `main/gnome/` and `scripts/`, making the overlay auditable and replaceable when upstream GNOME changes.

The icon reference artwork remains authoritative at `images/desktop-icons/set-002/`.
