# Ubuntu White — Desktop Look and Feel

## Look and Feel

`ubuntu-white` is a light, high-legibility desktop theme inspired by the open-source Ubuntu/GNOME desktop experience shown in the project reference image. It is an independent project style specification, not a claim of official Ubuntu branding or an attempt to reproduce proprietary artwork.

The visual target is:

- predominantly white workspace;
- dark-grey controls, labels, menus, and window chrome;
- white folder surfaces with restrained dark-grey outlines;
- named, legible icons rather than icon-only mystery controls;
- comfortable spacing and large hit targets;
- a modern, calm sans-serif font stack;
- subtle Ubuntu-like warmth through restrained accent use, without making the desktop orange-heavy;
- clear focus, selection, disabled, and error states;
- keyboard accessibility and visible focus indicators.

## Relationship to the Installer

The repository's Aptitude layer already treats desktop integration as a detectable installation surface and follows an evidence → plan → authorization → apply → verify model. The `ubuntu-white` package is therefore designed as a **Theme Payload**, not an installer that silently changes a machine.

A future installer should:

1. inspect the desktop environment;
2. identify supported GTK/icon/theme surfaces;
3. present the proposed theme changes;
4. obtain authorization for persistent changes;
5. install only the selected assets;
6. verify the resulting theme registration;
7. provide rollback to the previous theme where practical.

## Icon System

Icons are SVG-based and intentionally simple: white surfaces, dark-grey geometry, consistent stroke weight, generous negative space, and labels in the UI where ambiguity would otherwise result. The set begins with Home, Documents, Downloads, Music, Pictures, Videos, Public, Templates, Network, Terminal, Settings, Applications, and Trash.

The design should remain usable at 16, 24, 32, 48, and 64 px. SVG remains the source of truth so raster sizes can be generated without losing geometry.

## Font and Ergonomics

Preferred UI stack:

`Noto Sans, Ubuntu Sans, Inter, system-ui, sans-serif`

The theme favors approximately 1.45–1.55 line height, generous padding, consistent 8 px spacing increments, and 40–44 px minimum primary interactive targets where the host toolkit permits it.

## Legal and Moral Label

This work style is labelled **Moral and Guided by Law and Morals**. This is a project design value, not a claim that the theme itself creates legal rights or determines legal outcomes. Project-defined terminology does not replace statutory definitions.

## `.hsss`

The companion `.hsss` file is deliberately named **`.hsss`**, not `.hss`. Here it means **Human/System Style Specification**: a compact, human-readable declaration of visual intent, ergonomics, accessibility, provenance, and moral/legal design constraints.

Pronunciation: **H-S-S-S** (“aitch ess ess ess”).

`.hsss` is currently a project specification format; it is not presented as a standardized desktop toolkit format.
