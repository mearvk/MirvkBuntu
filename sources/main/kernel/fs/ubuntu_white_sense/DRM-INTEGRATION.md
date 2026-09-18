# DRM Integration — Ubuntu White Filesystem Class

`drm` is the removal-management companion for the Ubuntu White filesystem metadata class.

## Awareness

DRM is aware of the complete Ubuntu White metadata class and can address:

- the entire Ubuntu White metadata class;
- Sense layer 1;
- Sense layer 2;
- Sense layer 3;
- all Sense layers together.

The intended interface is:

```text
/drm --class ubuntu-white --sense 1 --dry-run
/drm --class ubuntu-white --sense 2 --dry-run
/drm --class ubuntu-white --sense 3 --dry-run
/drm --class ubuntu-white --sense all --dry-run
```

## Destructive boundary

The prototype deliberately **does not perform deletion**. A system-level command capable of deleting an entire filesystem metadata class is a high-impact operation and must have explicit authorization, scope confirmation, recovery/backup semantics, and atomic failure handling before it is enabled.

The current `drm` binary therefore acts as a removal planner/validator and requires `--dry-run`. This establishes the kernel interface without creating an accidental filesystem-destruction path.

## Relationship to COMB, LF, and MF

```text
COMB  -> collect / validate
LF    -> inspect / list
MF    -> modify metadata
DRM   -> plan authorized metadata removal
```

None of these tools may infer authority from a rating. Ubuntu White ratings are metadata only.

## EXT4

DRM targets the Ubuntu White metadata layer, not the ordinary EXT4 filesystem format. It must not delete an EXT4 filesystem, inode table, journal, or file payload merely because an Ubuntu White metadata class is selected.
