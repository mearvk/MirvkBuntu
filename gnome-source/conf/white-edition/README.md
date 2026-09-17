# Ubuntu White Edition GNOME Configuration

This is the human-readable distribution configuration layer for the Ubuntu White Edition GNOME desktop.

## Canonical configuration model

GNOME uses GSettings schemas with a settings backend such as dconf. System-wide defaults are expressed as dconf profiles and keyfiles and are compiled into dconf databases with `dconf update`.

This directory contains source configuration only. The ISO build installs it into the target filesystem and compiles the dconf database there.

## Layout

```text
gnome-source/conf/white-edition/
├── README.md
├── profile/
│   └── user
├── db/
│   └── local.d/
│       └── 00-white-edition
├── theme/
│   ├── README.md
│   ├── white-edition.css
│   ├── lighting.conf
│   ├── icons.conf
│   ├── icon-theme/
│   │   └── index.theme
│   └── install-icons.sh
├── boot/                          # boot-to-desktop presentation chain
│   ├── README.md
│   ├── ASSUMPTION.md              # OS-wide white-skinned-Ubuntu contract
│   ├── install-boot-presentation.sh
│   ├── grub/                      # white GRUB menu + default entry
│   ├── plymouth/white-edition/    # white boot splash
│   └── gdm/                       # white greeter + "Ubuntu White" session offering
├── shell-extension/
│   └── white-edition@mearvk/
│       ├── metadata.json
│       ├── extension.js
│       ├── prefs.js
│       ├── stylesheet.css
│       ├── install.sh
│       ├── README.md
│       ├── schemas/
│       │   └── org.gnome.shell.extensions.white-edition.gschema.xml
│       └── logos/
│           ├── circle-of-friends.svg
│           ├── mono-accent.svg
│           └── focus-ring.svg
└── install-config.sh
```

## Presentation is continuous from boot to desktop

The files at this level (`profile/`, `db/`, `theme/`) only style the desktop
**after** a session starts. The `boot/` subdirectory covers everything the user
sees **before** that — the GRUB menu, the boot splash, and the login greeter — so
the white-skinned presentation is never interrupted by stock Ubuntu styling.

```text
GRUB menu → Plymouth splash → GDM greeter (offers "Ubuntu White") → GNOME session
  boot/grub   boot/plymouth      boot/gdm                            db/ + theme/
```

The OS-wide assumption that the installed system is a white-skinned Ubuntu, and
the requirement that the **first boot after install presents the white-flavored
Ubuntu offering**, are specified in [`boot/ASSUMPTION.md`](boot/ASSUMPTION.md).
The build host installs the whole chain into an ISO target root with
[`boot/install-boot-presentation.sh`](boot/install-boot-presentation.sh) and then
runs the target-root activation steps (`update-grub`, `update-initramfs -u`,
`dconf update`) it prints.

## Desktop icon source of truth

The **initial Ubuntu White Edition Desktop LAF uses the artwork in `ubuntu-white/icons/`**. This repository path is authoritative for the initial desktop icon artwork; GNOME source trees are not an alternate icon source.

The icon tree currently contains root-level SVG artwork such as `folder.svg`, `home.svg`, `trash.svg`, `terminal.svg`, `settings.svg`, and `downloads.svg`, as well as development/working sets including `set-001`, `set-002`, `smaug`, and `svg-earlies`. The installation script intentionally does **not** install those working sets wholesale. Only explicitly approved root-level assets are mapped into the `Ubuntu-White` icon theme.

The mapping is currently:

```text
ubuntu-white/icons/folder.svg   → places/folder.svg
ubuntu-white/icons/home.svg     → places/home.svg
ubuntu-white/icons/trash.svg    → places/trash.svg
ubuntu-white/icons/terminal.svg → apps/terminal.svg
ubuntu-white/icons/settings.svg → apps/settings.svg
ubuntu-white/icons/downloads.svg → actions/downloads.svg
```

Additional icons should be added to the allow-list only after review. This prevents an unfinished icon set, build script, or unrelated directory from silently becoming part of the ISO's production LAF.

## Safe icon installation

`theme/install-icons.sh` takes an explicit ISO target root and refuses to operate without one. It requires the target to contain `/etc`, requires `ubuntu-white/icons` to exist, rejects symbolic links and special filesystem objects in the icon source, and copies only the approved artwork. It never deletes unrelated files from the target.

The installer creates `/usr/share/icons/Ubuntu-White/` in the target root and installs the repository-controlled `index.theme`. SVG transparency is preserved. The icon artwork itself remains under `ubuntu-white/icons/` as the source of truth.

The build system should run the icon installer before final dconf compilation/theme validation and should record the Git revision of the icon source in the ISO build manifest.

## Policy

- Prefer the light GNOME color scheme.
- Select `Ubuntu-White` as the icon-theme contract when the icon theme is installed.
- Keep settings user-overridable unless a specific lock is deliberately added.
- Do not copy compiled dconf databases into the repository.
- Keep module-specific policy in GSettings schemas and use this layer for distribution defaults.
- Do not place credentials, private databases, or machine-specific state here.
- Preserve the original icon artwork and use explicit allow-lists for production packaging.

## White Edition visual system

The desktop is intentionally predominantly white. Dark gray supplies readable text and controls; neutral gray supplies edges, shadows, and depth; Ubuntu red is a restrained active/focus highlight.

The visual system uses one stationary upper-left virtual key light. Icons receive a precise contact shadow, the bottom taskbar receives modest elevation, windows receive progressively softer shadows, and dialogs receive the greatest separation. During movement, the light remains stationary while object elevation changes. This produces a coherent 3D impression instead of unrelated decorative drop shadows.

The lighting specification is in `theme/lighting.conf`. The CSS/theme contract is in `theme/white-edition.css`. The icon source contract is in `theme/icons.conf`. These files describe the desired appearance; supported GNOME Shell, GTK, and compositor mechanisms remain responsible for implementing the actual effects.

The persistent taskbar/panel is a White Edition desktop requirement. Because GNOME Shell does not expose every layout behavior as a simple dconf preference, bottom placement should be implemented through the supported Shell extension/customization layer rather than by inventing a dconf key.

That Shell extension layer is provided by `shell-extension/white-edition@mearvk/`. It adds a Start button that stays fixed at the left of the panel (the panel left box) and a Start menu popup whose horizontal alignment (left, center, or right) is configurable through the `start-menu-alignment` GSettings key, with `left` as the default. The extension keeps the button anchored left and moves only the popup; it does not itself relocate the panel to the bottom of the screen, which remains the separate Shell-layout concern noted above. See `shell-extension/white-edition@mearvk/README.md` for the button-stays-left contract, the three selectable Ubuntu-themed logos, and install steps.

## Configuration ownership

Mutter owns compositor/window-management settings and compositor-level transitions; GNOME Shell owns shell behavior, panel/dash presentation, and extensions; GTK/GDK owns toolkit presentation; GLib/GSettings supplies the settings API and schemas; GVfs owns virtual filesystem backends; Control Center presents settings; GNOME Software and Terminal own their application settings; Orca owns accessibility preferences; glib-networking supplies networking/TLS integration. Cairo and GDK-Pixbuf are rendering/image libraries, Vala is build-time tooling, and Gala remains a separate optional compositor project.

## Build requirement

The build host should have `dconf` installed. After installing or changing keyfiles, the target root's dconf databases must be regenerated with `dconf update`. Theme and compositor changes must be built through the corresponding GNOME Shell, GTK, and Mutter integration layers.
