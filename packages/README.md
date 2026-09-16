# MirvkBuntu Basic Packages

`basic-packages.txt` is the first package layer for MirvkBuntu.

It is intentionally a **manifest**, not a copied package cache. Package files should be resolved from the selected Ubuntu archive and verified by the package manager.

## Layers

- Base command/runtime utilities
- Build and source acquisition tools
- Device/system integration
- Basic GNOME desktop runtime

Version pinning belongs in the MirvkBuntu release configuration once the target Ubuntu base is selected. Keeping package names separate from release metadata prevents the basic manifest from silently becoming tied to one archive snapshot.
