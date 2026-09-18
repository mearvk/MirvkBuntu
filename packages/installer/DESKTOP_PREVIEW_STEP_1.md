# Desktop Preview — Step 1

## Purpose

Step 1 establishes a smart, repeatable method for **seeing the desktop before committing to the installation design**. The goal is to make the desktop an observable design artifact that can be launched, inspected, changed, and tested without requiring a complete installed system.

## Step 1: Preview First

The installer should provide two paths:

```text
                    ISO BOOT
                       │
                 ┌─────┴─────┐
                 │  WELCOME  │
                 └─────┬─────┘
                       │
              ┌────────┴────────┐
              │                 │
           TRY DESKTOP       INSTALL
              │                 │
              ▼                 ▼
        DESKTOP PREVIEW    INSTALL PROFILE
              │                 │
              └────────┬────────┘
                       ▼
                  REVIEW
                       │
                    INSTALL
```

**Try Desktop** is the preferred design-review route. It should boot a live environment in which the complete desktop shell can be evaluated before disk changes are made.

## Smart Design Method

The desktop should be treated as a **profile-driven interface**, not as a collection of screenshots.

A preview profile should describe:

- display resolution and scaling;
- wallpaper/background;
- panel or taskbar placement;
- application launcher;
- system status area;
- terminal availability;
- file manager;
- settings/control center;
- accessibility options;
- theme and typography;
- default applications;
- network state;
- security state;
- installation-profile state.

The same profile should be usable by:

1. the live ISO preview;
2. a virtual machine;
3. a developer workstation;
4. the installed desktop;
5. screenshots and design review;
6. automated smoke tests where practical.

## Recommended Iteration Loop

```text
DESIGN
  ↓
PROFILE
  ↓
LIVE PREVIEW
  ↓
INSPECT
  ↓
CHANGE PROFILE
  ↓
REBUILD / RELOAD
  ↓
COMPARE
  ↓
ACCEPT
  ↓
INSTALL PROFILE
```

This makes the desktop **visible before it becomes permanent**.

## Install Profile Preview

The first preview should expose the installation profile as a normal desktop application or installer panel:

```text
┌──────────────────────────────────────────────────────────┐
│  UBUNTU DETERMINANT                                      │
│  Installation Profile                                    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  Welcome                                                 │
│                                                          │
│  Choose how this computer should begin.                  │
│                                                          │
│  ○  Try the system                                       │
│     Preview the desktop without installing               │
│                                                          │
│  ●  Install with Profile                                │
│     Review and configure the installation                │
│                                                          │
│  ──────────────────────────────────────────────────────  │
│                                                          │
│  Platform       Linux / x86_64                           │
│  Desktop        Desktop Profile                          │
│  Security       Standard / Hardened                      │
│  Updates        Enabled                                  │
│  Network        Automatic                                │
│  User           Create during installation                │
│  Storage        Review before writing                     │
│                                                          │
│                              [ Back ] [ Continue → ]      │
└──────────────────────────────────────────────────────────┘
```

The displayed values are examples. The implementation must obtain them from the actual profile rather than hard-code the illustration.

## Design Principles

### 1. No destructive action during preview

The live preview must not modify the target disk merely because the user is examining the desktop.

### 2. The profile is inspectable

A user should be able to determine what the installation intends to configure before accepting it.

### 3. Preview and installed desktop should converge

The desktop seen during **Try Desktop** should be substantially representative of the desktop obtained after installation, subject to hardware, drivers, user preferences, and first-run configuration.

### 4. Design changes should be cheap

Changing a desktop profile should not require redesigning the installer from scratch.

### 5. Virtualization is a second preview path

A VM image or automated VM boot should provide a fast development loop when rebooting physical hardware is inconvenient.

### 6. Screenshots are evidence, not the interface

Screenshots document a particular build. The profile and executable configuration define the actual interface.

## Acceptance Criteria for Step 1

Step 1 is complete when:

- the ISO can boot into a non-destructive desktop preview;
- the preview exposes the installation profile;
- the profile can be reviewed before installation;
- the desktop has a reproducible configuration source;
- the same configuration can be tested in a VM;
- a designer can change the profile and repeat the preview cycle;
- the installer can proceed from preview to installation without requiring a second unrelated desktop design.

## Next Step

**Step 2** should implement the actual preview environment and select the concrete desktop shell/window-manager strategy, then connect the first `Install Profile` screen to real installer configuration data.
