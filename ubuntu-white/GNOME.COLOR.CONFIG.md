# MirvkBuntu GNOME Color Configuration

## Purpose

This document defines configurable GNOME/Desktop settings derived from the MirvkBuntu Color-Dependent Theorem.

**Configuration file:** [ubuntu-white/color-inferencer.conf](https://github.com/mearvk/MirvkBuntu/blob/main/ubuntu-white/color-inferencer.conf)

The configuration file is the machine-readable source for the desktop color-inference parameters described below.

## Semantic roles

| Token | Default | Role |
|---|---|---|
| surface | #FFFFFF | Primary desktop surface |
| surface-alt | #F7F7F7 | Secondary surface |
| text | #111111 | Primary writing/font color |
| text-muted | #4A4A4A | Secondary writing |
| text-disabled | #8A8A8A | Disabled controls |
| primary | #D7263D | Active/selected/primary action |
| primary-pressed | #B01E31 | Pressed primary state |
| secondary | #0057B8 | Focus/link/informational state |
| secondary-hover | #0069DE | Secondary hover |
| border | #D6D6D6 | Structural boundary |
| on-accent | #FFFFFF | Text/icons over accents |

## GNOME settings

- GTK theme: MirvkBuntu-White
- GNOME Shell theme: MirvkBuntu-White
- color scheme: default
- interface font: Noto Sans 11
- document font: Noto Sans 11
- monospace font: Noto Sans Mono 11
- reduced visual ornament
- high-clarity focus indication
- semantic accent consistency

## Configurable variables

MIRVKBUNTU_COLOR_PRIMARY, MIRVKBUNTU_COLOR_SECONDARY, MIRVKBUNTU_COLOR_SURFACE, MIRVKBUNTU_COLOR_TEXT, MIRVKBUNTU_COLOR_BORDER, MIRVKBUNTU_COLOR_CHROMA_LIMIT, MIRVKBUNTU_COLOR_LIGHTNESS_STEP, and MIRVKBUNTU_COLOR_HUE_STEP may be overridden without changing semantic roles.

## Display-aware extension

A future Color Inferencer may evaluate the same semantic palette against sRGB, wide-gamut output, different display transfer functions, brightness levels, color-vision simulation, and ambient-light conditions. Semantic role must remain stable even when rendered RGB values change.
