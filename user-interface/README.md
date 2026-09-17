# White Edition User Interface

This directory defines the project UI foundation for the Ubuntu White Edition installer and `.asysma` control plane.

## Open-source reference study

The design study examined the open-source UI architecture of Chromium, Brave, and Firefox. Chromium exposes substantial browser UI through `chrome/browser/ui` and WebUI; Brave builds additional UI and WebUI layers on top of Chromium; Firefox maintains a dedicated desktop front-end and theme/design-system documentation. These are useful architectural references, not source files to copy wholesale.

- Chromium: BSD-licensed open-source browser project.
- Brave Core: MPL-2.0 modifications and UI/WebUI components layered around Chromium.
- Firefox: Mozilla open-source browser front-end with its own UI architecture and design system.

The project will **reimplement the useful interaction patterns rather than clone protected branding, artwork, trademarks, or unrelated implementation code**.

## Proposed White Edition UI

```text
Application shell
 ├── title / identity
 ├── navigation rail
 ├── workspace
 │    ├── Host
 │    ├── OS / Kernel
 │    ├── Stability
 │    ├── Security
 │    ├── AI / Capability
 │    ├── ISO
 │    ├── Install
 │    └── VM
 ├── status / audit strip
 └── action / confirmation surface
```

## Design goals

- white, professional, restrained visual language;
- strong hierarchy and readable typography;
- keyboard-first operation;
- accessible controls and clear focus states;
- explicit privilege and destructive-operation warnings;
- consistent status surfaces;
- modular components;
- Linux and Windows host support;
- JavaFX-first implementation for the installer;
- separation of UI from privileged native adapters.

## Reference principles

The strongest reusable ideas observed in the reference projects are:

1. **Componentized browser/application chrome.**
2. **Dedicated WebUI-style internal pages for complex administrative surfaces.**
3. **A shared icon and theme system.**
4. **Clear separation between presentation and system services.**
5. **Strong accessibility and keyboard navigation.**
6. **Platform-specific adaptation behind common interfaces.**
7. **Documentation alongside UI architecture.**

The resulting UI is original project work and is not represented as Chromium, Brave, or Firefox UI.

**Project attention:** Max Rupplin — MEARVK LLC — 2026.
