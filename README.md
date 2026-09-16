# MirvkBuntu

Ubuntu for Mearvk Users in the US the United States

## Project Foundation

MirvkBuntu is being organized as an operating-system project rather than as a single application. The first source layer establishes the basic Linux kernel, GNOME desktop foundation, and base package manifest.

### Basic Source Set

The initial source set is deliberately small and auditable:

- **Linux kernel:** 5.15.204, matching the kernel reference already carried by `Ubuntu.Determinant.Beta.Restricted`.
- **GNOME core:** Cairo, GLib, GDK-Pixbuf, GTK, Mutter, and GNOME Shell.
- **Basic packages:** base runtime, build, device, networking, and GNOME package names in `packages/basic-packages.txt`.

The large upstream source trees are **downloaded on demand** from their upstream locations rather than vendored into this repository. This keeps the MirvkBuntu Git history practical while retaining reproducible source references and revision records.

## Initial Layout

```text
MirvkBuntu/
├── bin/
├── boot/
├── desktop/
├── docs/
├── etc/
├── images/
├── installer/
├── kernel/
├── packages/
│   ├── README.md
│   └── basic-packages.txt
├── scripts/
├── sources/
│   ├── README.md
│   ├── kernel/
│   │   └── fetch-linux-kernel.sh
│   └── gnome/
│       └── fetch-gnome-core.sh
├── system/
├── tools/
└── README.md
```

The empty system directories will be populated only as their build responsibilities are defined.

## Acquiring the Basic Sources

From the repository root:

```bash
bash sources/kernel/fetch-linux-kernel.sh
bash sources/gnome/fetch-gnome-core.sh
```

The resulting source trees are placed under `sources/work/`, which is a local build workspace and is not intended for normal Git commits.

## Relationship to Ubuntu.Determinant.Beta.Restricted

The MirvkBuntu source layer starts with the reusable foundation already established in the Ubuntu.Determinant project. MirvkBuntu is the cleaner distribution-level home for the operating-system base; specialized research, experimental filesystems, and application-specific work remain in their existing projects until deliberately migrated.
