# MirvkBuntu Main Model

MirvkBuntu is a source-based Ubuntu-derived operating-system project for Mearvk users in the United States.

The model is layered:

```text
Hardware
   ↓
Linux kernel
   ↓
Core system / basic packages
   ↓
GNOME desktop
   ↓
MirvkBuntu integration and policy
   ↓
User applications
```

The kernel, GNOME, and basic package sources are treated as first-class source components. MirvkBuntu-specific changes belong above or alongside the upstream source and must be documented rather than obscured.

This directory describes the model; imported implementation source belongs in the corresponding source directories.
