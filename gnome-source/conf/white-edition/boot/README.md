# Ubuntu White Edition — Boot-to-Desktop Presentation Chain

This directory holds the source configuration that guarantees the **white-skinned
Ubuntu presentation is continuous from power-on through the first login and into
the desktop session**. It is the boot-time counterpart to the desktop dconf/theme
layer in `../` (`profile/`, `db/`, `theme/`).

The desktop layer (`../db/local.d/00-white-edition`, `../theme/`) only takes effect
*after* a user session starts. Everything a user sees **before** that — firmware
handoff, the GRUB menu, the boot splash, and the login greeter — is defined here so
the White Edition identity is never interrupted by stock Ubuntu/vendor styling.

## The presentation stages

```text
firmware / UEFI
      ↓
GRUB menu            ← boot/grub/theme/        (white menu, red highlight)
      ↓
Plymouth splash      ← boot/plymouth/white-edition/   (white splash during boot)
      ↓
GDM login greeter    ← boot/gdm/               (white greeter, White Edition session)
      ↓
GNOME Shell session  ← ../db + ../theme        (prefer-light, Ubuntu-White icons)
```

Each stage is a separately installable, separately reversible overlay. None of
them replace the upstream boot components; they supply White Edition **themes and
defaults** that the supported mechanisms (GRUB theme protocol, the Plymouth theme
system, and GDM/GSettings) load.

## Design tokens (shared with the desktop theme)

These match `../theme/white-edition.css` and `../theme/lighting.conf` exactly so the
boot chain and the desktop are visually one system:

| Token | Value | Use |
|---|---|---|
| surface | `#ffffff` | primary background |
| surface-raised | `#fafafa` | panels, selected rows |
| surface-soft | `#f3f3f3` | secondary fills |
| text | `#303030` | primary text |
| text-muted | `#666666` | secondary text |
| edge | `#dddddd` | borders / separators |
| accent (Ubuntu red) | `#e95420` | selection, focus, progress |

Lighting stays consistent with `../theme/lighting.conf`: a single stationary
upper-left key light, restrained gray shadows, no pure-black shadow, no glow.

## Contents

```text
boot/
├── README.md                     ← this file
├── ASSUMPTION.md                 ← OS-wide "this is white-skinned Ubuntu" contract
├── install-boot-presentation.sh  ← installs the whole chain into an ISO target root
├── grub/
│   ├── README.md
│   ├── 05_white_edition           ← /etc/default/grub.d drop-in (theme + gfx defaults)
│   └── theme/theme.txt            ← GRUB theme definition
├── plymouth/
│   └── white-edition/
│       ├── README.md
│       ├── white-edition.plymouth ← Plymouth theme descriptor
│       └── white-edition.script   ← script-module splash (white bg, red progress)
└── gdm/
    ├── README.md
    ├── 10-white-edition            ← GDM dconf defaults (greeter presentation)
    └── ubuntu-white.desktop        ← Xsession/Wayland session "Ubuntu White" offering
```

## Policy

- These are **defaults and themes**, not locks. A user or administrator can still
  choose a different session, disable the splash, or change GRUB behavior.
- The source artwork of record remains `ubuntu-white/icons/`; the boot chain refers
  to logo assets by install path rather than duplicating artwork here.
- Nothing here is compiled into the repository. The installer compiles/activates the
  themes inside the target root at build time (`dconf update`, `update-grub`,
  `update-initramfs -u`, `update-alternatives` for the Plymouth theme).
- Every stage records what it installed so it can be reverted and audited.

See `ASSUMPTION.md` for the OS-wide statement that Ubuntu White Edition is a
white-skinned Ubuntu by default, and how each layer upholds that assumption.
