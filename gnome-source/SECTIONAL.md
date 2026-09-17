# GNOME Source Sectional Build and Completeness Report

**Project:** Ubuntu Determinant Beta Restricted / Ubuntu White Edition  
**Tree:** `main/gnome-source`  
**Purpose:** Per-module inventory, canonical source layout, local build entry points, unusual-structure checks, and integration status.

## 1. Canonical layout

Every GNOME module now uses:

```text
gnome-source/<module>/
├── README.md
├── pull-source.sh
├── source/                 # canonical pristine upstream source
├── build/
│   └── build.sh            # local build entry point
└── patches/                # optional Determinant / White Edition offsets
```

`upstream/` is **not** a final source name. The ISO build consumes `source/` only.

The shared `build-module.sh` validates the source before compiling. It checks for a recognizable build target and rejects ambiguous structures where multiple top-level build systems are present. It does not silently choose an unusual target.

## 2. Local build policy

Each module has a module-specific `build/build.sh`. These scripts delegate to the common builder where the upstream build system is recognized. This keeps the module interface consistent while preserving upstream build definitions.

Before building, the common builder checks for:

- missing `source/`;
- no recognizable build definition;
- multiple competing top-level build definitions;
- missing required build tools;
- Autotools projects with `configure.ac` but no generated `configure`.

The builder creates a separate `build-local/` directory and does not place generated objects inside the pristine `source/` tree.

## 3. Module matrix

| Module | Role | Expected upstream build | Local entry point | Structure check | Status |
|---|---|---|---|---|---|
| Cairo | 2D graphics/rendering | Meson | `cairo/build/build.sh` | Common target validation | **BUILD-READY** |
| GDK-Pixbuf | Image loading/pixbuf | Meson | `gdk-pixbuf/build/build.sh` | Common target validation | **BUILD-READY** |
| GLib | Core GLib/GObject/GIO | Meson | `glib/build/build.sh` | Common target validation | **BUILD-READY** |
| glib-networking | GIO networking/TLS | Meson | `glib-networking/build/build.sh` | Common target validation | **BUILD-READY** after source normalization |
| GNOME Control Center | GNOME settings | Meson | `gnome-control-center/build/build.sh` | Common target validation | **BUILD-READY** after source normalization |
| GNOME Shell | Desktop shell | Meson | `gnome-shell/build/build.sh` | Common target validation | **BUILD-READY** |
| GNOME Software | Software management | Meson | `gnome-software/build/build.sh` | Common target validation | **BUILD-READY** after source normalization |
| GNOME Terminal | Terminal emulator | Meson | `gnome-terminal/build/build.sh` | Common target validation | **BUILD-READY** after source normalization |
| GTK | UI toolkit | Meson | `gtk/build/build.sh` | Common target validation | **BUILD-READY** |
| GVfs | GIO virtual filesystem services | Autotools/Meson depending on release | `gvfs/build/build.sh` | Common target validation | **BUILD-REVIEW** pending deliberate version/source normalization |
| Mutter | GNOME compositor/window manager | Meson | `mutter/build/build.sh` | Common target validation | **BUILD-READY** |
| Orca | GNOME accessibility | Meson/Python | `orca/build/build.sh` | Explicit Python/Meson check | **BUILD-REVIEW** |
| Vala | GNOME/GLib compiler | Autotools/Meson depending on release | `vala/build/build.sh` | Common target validation | **SOURCE-REVIEW** until actual source is present |
| Gala | Elementary OS compositor/window manager | Meson | `gala/build/build.sh` | Common target validation | **OPTIONAL / REVIEW** |

`Gala` remains a separate optional project and is never substituted for Mutter in the standard GNOME build.

## 4. Per-module build descriptors

### Cairo

**Build:** `gnome-source/cairo/build/build.sh`  
**Source:** `gnome-source/cairo/source/`  
**Role:** 2D rendering beneath GTK/GNOME.  
**Local checks:** verifies `source/`, detects the upstream Meson target, rejects competing top-level build systems, then configures and compiles in `build-local/`.

### GDK-Pixbuf

**Build:** `gnome-source/gdk-pixbuf/build/build.sh`  
**Source:** `gnome-source/gdk-pixbuf/source/`  
**Role:** image and pixbuf infrastructure, including desktop image/icon loading.  
**Local checks:** verifies a recognizable upstream build definition before compilation. White Edition PNG/JPEG assets remain outside pristine source.

### GLib

**Build:** `gnome-source/glib/build/build.sh`  
**Source:** `gnome-source/glib/source/`  
**Role:** core GLib, GObject and GIO APIs.  
**Local checks:** Meson target detection and ambiguity rejection.  
**Determinant relevance:** GIO is a principal higher-level integration point for the extended `...` and `....` path notation.

### glib-networking

**Build:** `gnome-source/glib-networking/build/build.sh`  
**Source:** `gnome-source/glib-networking/source/`  
**Role:** networking/TLS implementations for GIO.  
**Local checks:** Meson target detection.  
**Note:** older acquired trees were alpha/version-named; those must be normalized into `source/` before build.

### GNOME Control Center

**Build:** `gnome-source/gnome-control-center/build/build.sh`  
**Source:** `gnome-source/gnome-control-center/source/`  
**Role:** GNOME system settings.  
**Local checks:** Meson target detection.  
**White Edition relevance:** system theme and desktop configuration integration.

### GNOME Shell

**Build:** `gnome-source/gnome-shell/build/build.sh`  
**Source:** `gnome-source/gnome-shell/source/`  
**Role:** desktop shell, overview, panels and shell UI.  
**Local checks:** Meson target detection.  
**White Edition relevance:** primary shell-level visual/theme integration point.

### GNOME Software

**Build:** `gnome-source/gnome-software/build/build.sh`  
**Source:** `gnome-source/gnome-software/source/`  
**Role:** application/software management.  
**Local checks:** Meson target detection.

### GNOME Terminal

**Build:** `gnome-source/gnome-terminal/build/build.sh`  
**Source:** `gnome-source/gnome-terminal/source/`  
**Role:** terminal emulator.  
**Local checks:** Meson target detection.  
**Note:** acquired archives and versioned extraction directories are not final build boundaries; the normalized `source/` directory is.

### GTK

**Build:** `gnome-source/gtk/build/build.sh`  
**Source:** `gnome-source/gtk/source/`  
**Role:** GNOME UI toolkit.  
**White Edition relevance:** widgets, controls, typography, colors and visual rendering integration.

### GVfs

**Build:** `gnome-source/gvfs/build/build.sh`  
**Source:** `gnome-source/gvfs/source/`  
**Role:** virtual filesystem services and GIO backends.  
**Local checks:** detects the upstream build system rather than assuming one.  
**Review:** the previously acquired `1.9.5` source is an old generation and must not silently become the final GNOME 50-era dependency. Version selection must be deliberate.

### Mutter

**Build:** `gnome-source/mutter/build/build.sh`  
**Source:** `gnome-source/mutter/source/`  
**Role:** GNOME compositor and window manager.  
**White Edition relevance:** actual production GNOME display/compositing path. It is distinct from `isolation-desktop`, which remains a preview renderer.

### Orca

**Build:** `gnome-source/orca/build/build.sh`  
**Source:** `gnome-source/orca/source/`  
**Role:** GNOME accessibility/screen reader.  
**Local checks:** explicitly verifies that the source has a Meson or Python packaging target before attempting a build.

### Vala

**Build:** `gnome-source/vala/build/build.sh`  
**Source:** `gnome-source/vala/source/`  
**Role:** Vala compiler/language tooling used by GNOME/GLib development.  
**Review:** the repository previously contained a zero-length placeholder rather than a complete source tree; this remains a source-completeness gate until the actual source is acquired.

### Gala

**Build:** `gnome-source/gala/build/build.sh`  
**Source:** `gnome-source/gala/source/`  
**Role:** optional elementary OS compositor/window manager.  
**Local checks:** common target validation.  
**Important:** Gala is not Mutter and is not the standard GNOME compositor.

## 5. Unusual structure policy

The builder intentionally stops rather than guessing when it encounters a source tree with multiple competing top-level targets such as both `meson.build` and `CMakeLists.txt`.

It also stops when:

```text
source/ is absent
source/ is not a directory
no recognized build definition exists
configure.ac exists but configure is absent
required build tool is unavailable
```

This is preferable to accidentally compiling a nested example, test fixture, vendored dependency, or obsolete build target.

## 6. Build order

The modules should not be compiled in arbitrary alphabetical order. A practical dependency-oriented sequence is:

```text
Vala / toolchain prerequisites
        ↓
Cairo
        ↓
GLib
        ↓
GDK-Pixbuf
        ↓
GTK
        ↓
glib-networking / GVfs
        ↓
Mutter
        ↓
GNOME Shell
        ↓
GNOME Control Center / GNOME Software / GNOME Terminal / Orca
```

The ISO build system should eventually encode actual package-level dependency resolution rather than relying solely on this conceptual ordering.

## 7. White Edition integration

Pristine upstream source remains under `source/`. Ubuntu White Edition changes belong in `patches/` or an equivalent overlay.

The visual stack is:

```text
GNOME Shell
    ↓
Mutter
    ↓
GTK / GDK-Pixbuf
    ↓
Cairo
```

while the application/filesystem stack includes:

```text
GNOME applications
    ↓
GLib / GIO
    ├── GVfs
    └── glib-networking
```

Desktop PNG/JPEG icon assets should be integrated through the appropriate theme/resource mechanisms rather than replacing the upstream source tree.

## 8. Path notation extension

The existing Linux meaning of `..` remains unchanged.

The planned higher-level Determinant path policy is:

```text
..      one directory up
...     two directories up
....    three directories up
```

This should be integrated above the kernel VFS resolver, with GLib/GIO and the desktop file-management layer as appropriate integration points.

## 9. Completion gate

A module is production-ready only after the local build host demonstrates:

```text
[ ] source/ exists
[ ] source/ contains actual upstream source
[ ] expected build definition exists
[ ] unusual/ambiguous targets are rejected or explicitly handled
[ ] version/ref is recorded
[ ] checksum or commit is recorded
[ ] dependencies are available
[ ] configure/setup succeeds
[ ] compilation succeeds
[ ] tests appropriate to the module succeed
[ ] staging/install succeeds
[ ] ISO packaging includes the resulting component
[ ] White Edition theme/assets integrate successfully
[ ] pristine source remains recoverable
```

**Important:** these scripts are local build entry points. Their presence in Git does not by itself constitute a successful compilation. The build host must run them and preserve the resulting logs/status for the final release audit.
