# Ubuntu White File Copy Model

## Correction to the Sense model

Sense 1, Sense 2, and Sense 3 are **three physical file copies**, not three metadata records attached to one payload.

For a managed logical file `F`, the model is:

```text
F -> F.Sense1
  -> F.Sense2
  -> F.Sense3
```

Each copy is an independently addressable filesystem object and has its own ordinary filesystem metadata plus its own Ubuntu White 18-rating/health record.

## Storage consequence

The design deliberately spends additional disk capacity to obtain redundancy and richer file-level representation. In the simple fully materialized case, three copies require approximately 3x the payload storage before filesystem overhead, compression, deduplication, sparse-file effects, and block allocation are considered.

This is a **storage-for-information/redundancy tradeoff**, not a claim that storage itself creates human wealth. The project term `man-wealth-density` may be used as a conceptual measure of information recorded per unit of managed storage, but it is not a filesystem primitive.

## Kernel boundary

The kernel/filesystem layer should treat the three copies as a managed copy set with explicit relationships. Ordinary EXT4 files remain ordinary files. Native copy-set enforcement requires filesystem-specific implementation and recovery semantics; the current prototype documents the model without modifying the EXT4 on-disk format.

## Copy integrity

Each copy should eventually have an independent content identity/checksum and lifecycle state. `comb` can compare the copies; `lf` can display their common data and per-copy ratings; `mf` can modify permitted metadata for a selected copy; and `drm` can remove a selected copy or an entire authorized Ubuntu White copy set.

Deletion of one copy must not implicitly delete the others unless an explicit whole-set operation is requested and authorized.
