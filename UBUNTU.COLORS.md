# UBUNTU.COLORS.md — MirvkBuntu Desktop Binding Colors

This document defines the **binding color palette** for the MirvkBuntu desktop.
It is the authoritative color contract for the operating system's user
interface: themes, icons, the display manager, and application chrome should
bind to these colors rather than inventing their own.

## Core palette

MirvkBuntu's desktop is built on four binding colors:

| Role | Color | Hex | RGB | Usage |
|------|-------|-----|-----|-------|
| Primary | **Red** | `#D7263D` | `215, 38, 61` | Primary accent — selection, active controls, focus, brand highlights |
| Secondary | **Blue** | `#0057B8` | `0, 87, 184` | Secondary accent — links, informational highlights, progress |
| Surface | **White** | `#FFFFFF` | `255, 255, 255` | Backgrounds, panels, window surfaces, empty space |
| Text | **Black** | `#111111` | `17, 17, 17` | All writing and font-related items: body text, labels, icon glyphs |

> **Rule:** Red, blue, and white are the desktop's structural colors. **Black
> (`#111111`) is reserved for writing / font-related items** — body text,
> headings, labels, and monochrome icon glyphs. Do not use red or blue as the
> default text color; use them as accents on top of white surfaces.

## Roles and application

### White — surfaces
White is the default surface color: window backgrounds, panels, sheets, menus,
and the shell background. It carries the "Ubuntu White" identity of the desktop.

- Window / panel background: `#FFFFFF`
- Secondary surface (cards, sidebars): `#F7F7F7`
- Divider / hairline on white: `#D6D6D6`

### Black — text and fonts
Black is used **only** for writing and font-related items so text is maximally
legible on white surfaces.

- Body text: `#111111`
- Muted / secondary text: `#4A4A4A`
- Disabled text: `#8A8A8A`

Never render primary body text in red or blue.

### Red — primary accent
Red is the primary brand accent and marks the most important interactive state.

- Selection / active tab / focused control: `#D7263D`
- Pressed state: `#B01E31`
- Destructive action (delete, stop): `#D7263D`
- Text/icon placed on a red fill: **White** `#FFFFFF`

### Blue — secondary accent
Blue is the secondary accent for informational and navigational elements.

- Hyperlinks: `#0057B8`
- Informational highlight / progress: `#0057B8`
- Hover on blue: `#0069DE`
- Text/icon placed on a blue fill: **White** `#FFFFFF`

## Contrast and accessibility

Text and interactive colors must meet WCAG AA contrast on their intended
surface:

- Black text `#111111` on white `#FFFFFF` → ratio ≈ 18.9 : 1 (passes AAA).
- White text `#FFFFFF` on red `#D7263D` → ratio ≈ 4.96 : 1 (passes AA for normal text).
- White text `#FFFFFF` on blue `#0057B8` → ratio ≈ 6.87 : 1 (passes AA).

Red and blue accents are used for emphasis, not for long-form reading. Long-form
reading text is always black on white.

## Reference tokens

Suggested token names for theme bindings (GTK CSS custom properties, icon theme
palettes, and the display manager):

```text
--mirvk-color-primary        #D7263D   /* red   */
--mirvk-color-primary-pressed #B01E31
--mirvk-color-secondary      #0057B8   /* blue  */
--mirvk-color-secondary-hover #0069DE
--mirvk-color-surface        #FFFFFF   /* white */
--mirvk-color-surface-alt    #F7F7F7
--mirvk-color-border         #D6D6D6
--mirvk-color-text           #111111   /* black — fonts/writing */
--mirvk-color-text-muted     #4A4A4A
--mirvk-color-text-disabled  #8A8A8A
--mirvk-color-on-accent      #FFFFFF   /* text/icons on red or blue */
```

## Implementation

This contract is implemented by the **MirvkBuntu-White** theme in
`ubuntu-white/`:

- `ubuntu-white/gtk.css` — GTK 3/4 theme; defines the tokens above as
  `@define-color` and maps GTK's standard named colors onto them.
- `ubuntu-white/gnome-shell.css` — GNOME Shell theme (panel, overview, dash,
  menus) using the same palette inline.

The build installs this as `/usr/share/themes/MirvkBuntu-White` and sets it as
the default GTK + Shell theme via a system dconf database, so both the quick
remaster (`build/quick-remaster.sh`) and the slim/native GNOME editions
(`build/build-slim.sh`) boot with the palette applied. This is a solid baseline
("good norm") intended to be refined over time.

## Notes

- These colors are the binding contract; the `ubuntu-white/` theme assets track
  them. When adjusting the palette, change the values here and in the theme's
  token definitions together.
- On accent fills (red or blue), text and glyphs switch to white for legibility;
  everywhere else, writing stays black on white.
