# Ubuntu White Edition — OS Presentation Assumption

**Project:** Ubuntu Determinant Beta Restricted / Ubuntu White Edition
**Scope:** Whole-OS visual presentation, power-on through desktop session
**Status:** DESIGNED / SPECIFIED (boot chain source present; activation is a build-host step)

## 1. The assumption

Ubuntu White Edition assumes, as a baseline OS property, that **the installed
operating system is a white-skinned Ubuntu**. "White-skinned" here means the
White Edition visual system — predominantly white surfaces, dark-gray text,
restrained neutral-gray depth, and a single restrained Ubuntu-red accent — is the
**default presentation of every stage the user sees**, not a theme the user must
discover and enable after installation.

This is a *default*, not a lock. The system remains fully Ubuntu: users and
administrators can change the session, theme, splash, or boot behavior. The
assumption governs the out-of-the-box state, specifically the state immediately
**after install, on first boot**.

## 2. Where the assumption must hold

The presentation must be continuous. There must be no stage at which stock Ubuntu,
vendor firmware branding, or an unstyled default interrupts the White Edition
identity between power-on and the desktop.

```text
Stage                     Assumption upheld by                         State
─────────────────────────────────────────────────────────────────────────────
1. Firmware / UEFI        (out of project scope — vendor firmware)     N/A
2. GRUB boot menu         boot/grub/theme + boot/grub/05_white_edition WHITE
3. Early boot / splash    boot/plymouth/white-edition/                 WHITE
4. Login greeter (GDM)    boot/gdm/10-white-edition                    WHITE
5. Session offering       boot/gdm/ubuntu-white.desktop                "Ubuntu White"
6. Desktop session        ../db/local.d/00-white-edition + ../theme/   WHITE
─────────────────────────────────────────────────────────────────────────────
```

Stage 1 (firmware) is outside the OS: the project does not repaint vendor firmware.
Stages 2–6 are all White Edition responsibilities and are all supplied in this
repository.

## 3. The first-boot offering

The specific requirement in scope is: **the bootup after install presents the
white-flavored Ubuntu offering.** Concretely, on the first boot of a freshly
installed system:

1. GRUB shows the White Edition menu (white background, dark-gray entries, Ubuntu-red
   highlight on the selected entry). "Ubuntu White Edition" is the default, top entry.
2. The boot splash is the White Edition Plymouth theme (white field, centered mark,
   Ubuntu-red progress), not the stock spinner.
3. The GDM greeter is white-skinned, and the session chooser **offers a session named
   "Ubuntu White"** (`boot/gdm/ubuntu-white.desktop`), selected as the default session.
4. Logging into that session lands in the White Edition GNOME desktop
   (`prefer-light`, `Ubuntu-White` icon theme, White Edition CSS/lighting).

If any of stages 2–5 falls back to stock presentation, the assumption is considered
**violated** and the build audit (see `../../SECTIONAL.md` §9 completion gate) should
record it rather than shipping.

## 4. Ownership map

Each stage is owned by a specific mechanism; the White Edition layer only supplies
defaults/themes to that mechanism. This mirrors the ownership map in
`../README.md` ("Configuration ownership") and extends it to boot time:

| Stage | Upstream owner | White Edition input |
|---|---|---|
| GRUB menu | GRUB 2 | `grub/theme/theme.txt`, `grub/05_white_edition` drop-in |
| Boot splash | Plymouth | `plymouth/white-edition/` theme |
| Greeter | GDM + GSettings | `gdm/10-white-edition` dconf defaults |
| Session list | XDG session `.desktop` + GNOME Shell | `gdm/ubuntu-white.desktop` |
| Desktop | GNOME Shell / GTK / Mutter / dconf | `../db`, `../theme` (existing) |

## 5. What this assumption does NOT claim

- It does **not** claim White Edition owns or forks GRUB, Plymouth, or GDM. They
  remain upstream components; only theme/default overlays are added.
- It does **not** remove the user's ability to pick another session, theme, or
  disable the splash.
- It does **not** repaint firmware/UEFI vendor screens.
- It does **not** assert the boot chain is already *activated* — the source is
  present here; the build host must run `install-boot-presentation.sh` against the
  ISO target root and then run the target-root activation commands
  (`update-grub`, `update-initramfs -u`, `dconf update`) for the assumption to hold
  on a real image.

## 6. Verification checklist (build-host)

The boot presentation is upheld only after the build host demonstrates, on the
target image:

```text
[ ] GRUB uses the White Edition theme and lists "Ubuntu White Edition" as default
[ ] GRUB gfxmode/gfxpayload set so the theme renders (not text fallback)
[ ] Plymouth default theme = white-edition (update-alternatives / plymouth-set-default-theme)
[ ] initramfs rebuilt so the Plymouth theme is present in early boot
[ ] GDM greeter shows White Edition presentation (light, Ubuntu-White accents)
[ ] Session chooser offers "Ubuntu White" and it is the default session
[ ] First login lands in the White Edition GNOME desktop (prefer-light + Ubuntu-White)
[ ] No stage falls back to stock Ubuntu presentation
[ ] Each installed file is recorded for reversibility/audit
```

---

**Max Rupplin — MEARVK LLC — 2026**
