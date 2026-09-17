# GNOME Source Reference

This directory contains the GNOME and GNOME-adjacent source components used by Ubuntu.Determinant.Beta.Restricted / Ubuntu White Edition.

## Canonical source layout

Every module uses the same source boundary:

```text
gnome-source/<module>/
├── README.md
├── pull-source.sh
├── source/               # canonical upstream source tree
├── build/                # local build wrappers and checks
└── patches/              # optional Determinant offsets/patches
```

**`source/` is the final source directory name.** The older `upstream/` name is transitional only and must not be used as the production source boundary.

If an older acquisition still leaves an `upstream/` directory, run:

```bash
./gnome-source/normalize-source-layout.sh
```

The normalizer migrates an acquired `upstream/` tree to `source/` without changing source contents.

## Desktop flow: left to right

The modules are organized below in approximate dependency / desktop-flow order. This is a conceptual flow, not a claim that every module is a direct dependency of the next one.

```text
Cairo → GLib → GDK-Pixbuf → GTK → Mutter → GNOME Shell
   │       │         │          │        │          │
   │       │         │          │        │          ├─ desktop UI / shell
   │       │         │          │        └─ display, windows, compositor
   │       │         │          └─ widgets / application UI
   │       │         └─ image loading / pixel assets
   │       └─ core types, I/O, IPC, utilities
   └─ 2D rendering

GLib → glib-networking → GVfs → GNOME applications
                    │       │
                    │       └─ virtual filesystem / remote resources
                    └─ TLS/networking support

Vala → GNOME libraries / applications
       └─ compiler and language tooling used by GNOME software

GNOME Shell → GNOME Control Center
            → GNOME Software
            → GNOME Terminal
            → Orca

Gala → optional alternative compositor/window-manager project
       (not a replacement for Mutter in the GNOME build)
```

Mutter is the central display/compositing component used by GNOME Shell; current GNOME documentation describes it as a Wayland display-server compositor library with Xwayland, window management, compositing, focus, workspaces, keybindings, and monitor configuration. citeturn0search8 GNOME's technology documentation likewise describes GNOME Shell as building on Mutter and the GNOME platform libraries. citeturn0search3

## Module reference

### 1. Cairo

**Purpose:** 2D vector/raster rendering library used beneath much of the desktop graphics stack.

**Role:** Rendering foundation for GTK/Pango and other graphics consumers. GNOME's historical GTK documentation identifies Cairo as a required component in the GTK graphics stack. citeturn0search2

**Complexity:** 8/10. Low-level rendering, multiple output backends, geometry, compositing, rasterization, and performance constraints make this foundational code technically demanding.

**Age:** Project dates to the early 2000s; it has been part of the Linux/free-desktop graphics stack for more than two decades.

**People / stewardship:** Cairo has had many contributors over its lifetime; the repository should treat upstream copyright and AUTHORS/maintainer records as authoritative rather than naming a single creator.

**Education / institutional affiliation:** Only publicly documented professional affiliations should be recorded. Educational history is not consistently published for the full contributor set and should not be inferred.

**Source size:** Record the exact local source size at build time with the module's build/audit tooling. Do not hard-code a size here because Git checkout state and generated files change it.

### 2. GLib

**Purpose:** Core GNOME/GLib platform library providing data structures, event loops, threading primitives, GObject, GIO, subprocess and I/O facilities, settings-related infrastructure, and other common facilities.

**Role:** One of the primary foundations of the entire GNOME stack.

**Complexity:** 10/10. It is a large, long-lived foundational platform with APIs consumed by a very large number of components.

**Age:** GLib originated in the GTK ecosystem in the 1990s and has therefore existed for roughly three decades.

**People / stewardship:** The project has a broad contributor base. Public GNOME material identifies maintainers and contributors including Patrick Griffis and historically Ryan Lortie, Matthias Clasen, Colin Walters, and others. citeturn0search12turn0search1

**Education / institutional affiliation:** Public professional affiliations may be recorded where explicitly published; educational institutions are not assumed from names or biographies and should be added only when reliably documented.

**Source size:** Measure from the checked-out `source/` tree during the local build audit.

### 3. GDK-Pixbuf

**Purpose:** Image-loading and pixel-buffer infrastructure used by GTK and GNOME applications.

**Role:** Supplies decoded image data and image-loader integration for UI assets and applications.

**Complexity:** 7/10.

**Age:** Originates in the long-running GTK/GNOME desktop stack and has been maintained across multiple GTK generations.

**People / stewardship:** Multi-contributor GNOME project; use the module's AUTHORS/maintainer metadata as the authoritative contributor record.

**Education / institutional affiliation:** Include only public professional or educational information explicitly documented by the individuals themselves or authoritative project biographies.

**Source size:** Measure locally from `source/`.

### 4. GTK

**Purpose:** The principal widget and UI toolkit for GNOME applications, with GDK providing display/input integration.

**Role:** Defines much of the visible application UI: widgets, controls, layout, accessibility interfaces, input, rendering integration, and platform backends.

**Complexity:** 10/10. GNOME's archived technical documentation describes GTK+ as a roughly 600 KLOC codebase at that point in its history, with many subsystems and authors. citeturn0search0

**Age:** GTK began in the late 1990s and has existed for approximately three decades.

**People / stewardship:** The historical GNOME documentation identifies contributors including Owen Taylor, Matthias Clasen, Benjamin Otte, Alex Larsson, Carlos Garnacho, Kristian Rietveld, Ryan Lortie, John Palmieri, and others across individual subsystems. citeturn0search0

**Education / institutional affiliation:** Some contributors have public professional biographies, but education is not uniformly documented. Do not infer colleges or affiliations from names.

**Source size:** Measure locally; historical KLOC figures are not a substitute for the exact source checkout used here.

### 5. Mutter

**Purpose:** Wayland display-server compositor and X11/Xwayland window-management/compositing infrastructure.

**Role:** Provides display management, compositing, window management, workspaces, focus, keybindings, monitor configuration, and the low-level rendering stack consumed by GNOME Shell. citeturn0search8

**Complexity:** 10/10. It combines graphics, input, window management, display protocols, hardware acceleration, and compositor behavior.

**Age:** Evolved from Metacity and the Clutter-based Mutter project in the late 2000s; roughly two decades of development.

**People / stewardship:** Current GNOME component documentation lists Jonas Ådahl, Carlos Garnacho, Georges Basile Stavracas Neto, and Florian Müllner among maintainers. citeturn0search11

**Education / institutional affiliation:** Record only publicly documented professional affiliations; do not infer educational history.

**Source size:** Measure locally from `source/`.

### 6. GNOME Shell

**Purpose:** The primary GNOME desktop shell and user-facing desktop environment layer.

**Role:** Desktop overview, panels, application launching, shell UI, notifications, session interaction, and JavaScript-based shell behavior on top of Mutter and GNOME platform libraries. citeturn0search3

**Complexity:** 9/10.

**Age:** GNOME Shell was introduced as part of the GNOME 3 generation around 2011 and has been continuously developed since then.

**People / stewardship:** Current GNOME component documentation lists Florian Müllner and Georges Basile Stavracas Neto as maintainers. citeturn0search11

**Education / institutional affiliation:** Only public professional biographies should be used; no inferred personal information.

**Source size:** Measure locally from `source/`.

### 7. glib-networking

**Purpose:** Network-security and networking integration for GLib/GIO, including TLS-related functionality.

**Role:** Provides networking support used by applications and GNOME platform components.

**Complexity:** 7/10.

**Age:** Developed as part of the modern GLib/GIO networking stack during the GNOME 2/3 era.

**People / stewardship:** Multi-contributor GNOME project; consult its source metadata for the exact maintainers associated with the selected revision.

**Education / institutional affiliation:** No inference; record only publicly documented information.

**Source size:** Measure locally.

### 8. GVfs

**Purpose:** Userspace virtual filesystem implementation for GIO.

**Role:** Provides local/remote storage integration, mounts, metadata, trash support, SFTP, SMB, HTTP, DAV and other backends, plus related GIO integration. citeturn1search14turn1search15

**Complexity:** 9/10 because it crosses filesystem, networking, IPC, authentication, storage, and desktop integration boundaries.

**Age:** GVfs replaced the older gnome-vfs approach around the GNOME 2.22/2.24 period, giving it roughly 18 years of history.

**People / stewardship:** Historical GNOME documentation identifies Christian Kellner and Alexander Larsson among GVfs maintainers. citeturn1search2 Current GNOME component material lists Ondrej Holy, Philip Langdale, and Jan-Michael Brummer. citeturn1search20

**Education / institutional affiliation:** Use only public biographies; no inferred personal information.

**Source size:** Measure locally. The repository's `SECTIONAL.md` must additionally verify that the selected GVfs source revision is appropriate rather than accepting an unexpectedly old checkout.

### 9. GNOME Control Center

**Purpose:** Central graphical settings application for GNOME.

**Role:** Provides the user-facing settings panels for display, networking, accounts, power, keyboard, mouse, privacy, applications, and related desktop configuration.

**Complexity:** 8/10.

**Age:** GNOME Control Center has existed since the early GNOME desktop generations and the modern GNOME 3 implementation dates back to the early 2010s.

**People / stewardship:** Current GNOME module information lists Carlos Garnacho, Felipe Borges, Bastien Nocera, Marek Kasik, and Matthijs Velsink as maintainers. citeturn1search0

**Education / institutional affiliation:** Only publicly documented professional affiliations should be included.

**Source size:** Measure locally.

### 10. GNOME Software

**Purpose:** Graphical software discovery, installation, updates, and application-management interface.

**Role:** User-facing software center integrated with package and application metadata systems.

**Complexity:** 8/10 because it bridges application metadata, package systems, Flatpak, updates, repositories, policy, and UI.

**Age:** GNOME Software development began in the early 2010s; the project was publicly described as an active GNOME application by 2013 and released in the GNOME 3.12 era. citeturn1search8turn1search6

**People / stewardship:** Current Apps for GNOME lists Milan Crha, Philip Withnall, and Richard Hughes as maintainers. citeturn1search16 Richard Hughes has publicly stated that he graduated from the University of Surrey with a Master's in Electronics Engineering and has worked in the Red Hat desktop group. citeturn1search5

**Education / institutional affiliation:** Include only explicitly public information such as the preceding biography; do not infer additional affiliations.

**Source size:** Measure locally.

### 11. GNOME Terminal

**Purpose:** Terminal emulator for running UNIX/Linux shells and command-line programs.

**Role:** User-facing terminal application supporting text interaction, tabs, profiles, keyboard shortcuts, colors, and terminal escape sequences. citeturn1search17turn1search18

**Complexity:** 7/10.

**Age:** GNOME Terminal dates to the early GNOME desktop era and has been maintained through successive GNOME generations.

**People / stewardship:** Current GNOME module information identifies Christian Persch as maintainer. citeturn1search1

**Education / institutional affiliation:** No unverified educational or personal-affiliation data is included.

**Source size:** Measure locally.

### 12. Orca

**Purpose:** GNOME screen reader and accessibility technology for blind and visually impaired users.

**Role:** Converts application and desktop accessibility information into speech and other assistive output.

**Complexity:** 8/10 because it crosses accessibility APIs, AT-SPI, applications, text, events, speech, input, and desktop integration.

**Age:** Orca originated in the GNOME accessibility ecosystem in the early 2000s.

**People / stewardship:** GNOME component documentation lists Joanmarie Diggs and Federico Mena Quintero among maintainers. citeturn0search11

**Education / institutional affiliation:** Record only publicly documented professional information; do not infer personal beliefs or affiliations.

**Source size:** Measure locally.

### 13. Vala

**Purpose:** Programming language and compiler that generates C code for GLib/GNOME development.

**Role:** Build-time/development tooling rather than a desktop runtime component.

**Complexity:** 8/10 because it includes language semantics, compiler infrastructure, C generation, introspection and GLib/GObject integration.

**Age:** Vala was introduced in the GNOME ecosystem in the mid-2000s and has roughly two decades of history.

**People / stewardship:** The project has had many contributors; historical GNOME records identify contributors including Luca Bruno among Vala developers. citeturn0search9

**Education / institutional affiliation:** Only publicly documented information should be recorded.

**Source size:** Measure locally. The build audit must reject a placeholder or zero-length `source/` directory.

### 14. Gala

**Purpose:** Window manager/compositor project associated with elementary OS.

**Role:** Optional alternative desktop compositor/window-manager technology. It is **not GNOME Mutter** and should not be represented as the production GNOME compositor.

**Complexity:** 8/10.

**Age:** Gala is associated with the elementary desktop project and has been developed through multiple elementary OS generations.

**People / stewardship:** Use the Gala project's own contributor and maintainer records for the exact revision selected by this repository.

**Education / institutional affiliation:** Do not infer personal education, religion, politics, or other sensitive characteristics. Record only public professional/project affiliations.

**Source size:** Measure locally.

## People, authorship, education and affiliations policy

This document records **software stewardship**, not personal dossiers. For each module, the preferred people data is:

1. official maintainers;
2. clearly documented major contributors where relevant to the module;
3. public professional/project affiliations;
4. educational institutions only when the person has publicly and reliably documented that information.

Religious beliefs, political affiliations, health information, sexual orientation, and other sensitive personal characteristics are **not included or inferred**. A person's name, nationality, employer, university, or project affiliation must never be used to guess such characteristics.

GNOME itself describes modules as maintained by one or more maintainers, with maintainers responsible for accepting changes and making releases. citeturn1search7

## Size and complexity measurement

`Complexity 1–10` in this document is an engineering-maintenance assessment for this OS integration, not a measure of the intelligence or worth of the people who wrote the software.

Exact module size must be measured from the checked-out source used by the local build. Each build script should eventually record:

```text
source bytes
source files
C/C++/Vala/Python/JS/other line counts where useful
build artifacts
installed footprint
build duration
compiler/toolchain version
upstream revision
patch count
```

This prevents stale historical LOC estimates from being presented as the size of the actual source checkout.

## Local build contract

Every module is expected to provide:

```text
gnome-source/<module>/
├── source/
├── build/
│   └── build.sh
├── patches/
├── README.md
└── pull-source.sh
```

The build wrapper must check for unusual source layouts, missing build targets, conflicting build systems, missing tools, and placeholder source trees before attempting compilation. Generated objects must remain outside `source/`.

## Ubuntu White Edition integration

The GNOME source is treated as the base platform. Ubuntu White Edition changes should be applied through explicit patch/offset layers for:

- desktop colors and theme;
- icons and folder assets;
- GTK/GNOME visual styling;
- window/compositor behavior where required;
- desktop launchers and application presentation;
- accessibility configuration;
- filesystem/UI behavior.

The source boundary remains `source/`; the production build must never silently replace it with `upstream/` or a generated directory.

## Audit

See `SECTIONAL.md` for the authoritative per-module source/build audit and the local compilation completion status.
