# Ubuntu White Edition — GNOME Theme

This directory is the visual customization layer for the production GNOME desktop.

It deliberately does not fork GNOME Shell, Mutter, or Nautilus merely to change appearance. Upstream GNOME remains the runtime; this layer supplies Ubuntu White Edition artwork, GTK styling, GNOME Shell styling, backgrounds, and desktop settings.

## Design goals

- Professional Ubuntu White Edition appearance.
- White/light surfaces with restrained dark-gray UI text and controls.
- Use the project's curated Ubuntu White icon artwork from `main/ubuntu-white/icons/`.
- Preserve transparent icon backgrounds where the artwork provides them.
- Keep GNOME upgrades separable from our visual identity.
- Avoid changing behavior until a theme, setting, or extension cannot accomplish the desired result.

## Layout

- `gtk/` — GTK application styling and settings.
- `shell/` — GNOME Shell CSS and shell presentation.
- `icons/` — GNOME-compatible installed icon-theme structure and project icon integration.
- `backgrounds/` — desktop background assets and metadata.
- `gsettings/` — reproducible GNOME appearance defaults.
- `extensions/` — optional behavior/UI extensions; keep these small and documented.

The preview implementation in `main/isolation-desktop/` is not the authoritative production renderer.
