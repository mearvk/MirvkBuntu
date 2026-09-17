# White Edition Precision Theme

This directory defines the visual language for the Ubuntu White Edition GNOME desktop.

## Design objective

The desktop should read as a professional, predominantly white physical surface with controlled depth. Gray provides shadow, edge, and elevation information. Dark gray provides readable text and controls. Ubuntu red is reserved for focus, active state, and deliberate highlights.

## Lighting model

`lighting.conf` defines a single stationary upper-left virtual key light. Desktop objects receive different elevations and shadow softness according to their role:

1. Desktop surface — baseline.
2. Icons — low elevation and tight contact shadow.
3. Bottom taskbar/panel — modest elevation.
4. Normal windows — larger, softer separation.
5. Focused windows — slightly greater elevation and restrained red focus treatment.
6. Dialogs/menus — highest visual elevation.

During movement, the light remains stationary. The object changes elevation and its shadow responds; the shadow must not appear attached as a decorative bitmap.

## Implementation boundary

`white-edition.css` is a theme-layer specification. Actual GNOME Shell, GTK, and compositor selectors/APIs must be implemented by the corresponding supported component. `lighting.conf` is a design contract and is not itself a GNOME API.

Mutter is responsible for compositor-level window effects and transitions. GNOME Shell is responsible for desktop/panel presentation. GTK is responsible for application widget presentation. The White Edition icon package supplies the icon artwork.

## Quality rules

- No pure-black drop shadows.
- No uncontrolled glow.
- No arbitrary per-widget light directions.
- Preserve text contrast and accessibility.
- Prefer subtle physical depth over glossy decoration.
- Keep red accents sparse and intentional.
- Maintain consistent apparent light direction across icons, windows, menus, and panels.
