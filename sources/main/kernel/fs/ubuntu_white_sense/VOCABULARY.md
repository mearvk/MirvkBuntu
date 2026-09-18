# Ubuntu White Filesystem Vocabulary

## Core terms

- **Ubuntu White** — the repository's filesystem metadata profile; it adds metadata without changing ordinary file payloads.
- **Sense** — an ordered metadata layer associated with a file. A managed file has Sense 1, 2, and/or 3; each layer has its own generic ratings and overall health.
- **Sense layer** — one independent rating/health record, numbered 1 through 3.
- **Generic rating** — a bounded metadata value associated with one of the 18 schema-defined rating names. It is not an executable instruction or a judgment about a person.
- **Overall health** — a file/Sense metadata integrity and lifecycle indicator. It is not a measure of human worth, intelligence, status, or authority.
- **Hold type** — an extensible generic classification attached to a file record. It has no authority by itself.
- **COMB** — the read-mostly collector and validator for Ubuntu White metadata.
- **LF (`lf`)** — a companion to `ls` that lists common file information and, with explicit flags, Sense metadata.
- **MF (`mf`)** — metadata modifier for file identity/name/date/author/database and Sense metadata. It changes metadata, not payload, in the prototype.
- **DRM** — the Ubuntu White metadata-removal interface. Its scope is the Ubuntu White metadata class, not arbitrary filesystem destruction.
- **File payload** — the actual bytes/content of a file, distinguished from its metadata.
- **Sidecar/index** — the initial metadata storage boundary used to remain compatible with stock EXT4.
- **EXT4** — the underlying Linux filesystem targeted by the initial compatibility design; native on-disk changes are not part of the prototype.
- **Installer Profile** — the operating-system installation configuration that may initialize metadata for newly installed files.

## Generic rating vocabulary

The schema contains exactly 18 generic ratings:

1. `use`
2. `age`
3. `homo`
4. `homotype`
5. `useage`
6. `manage`
7. `action`
8. `lists`
9. `calls`
10. `actionsagainst`
11. `same`
12. `came`
13. `come`
14. `hold`
15. `research`
16. `archer-class`
17. `master-manager-class`
18. `imperial-calls`

These names are schema vocabulary only. Implementations must not assign unrequested social, political, legal, medical, or other sensitive meanings to them.

## Command vocabulary

- `lf FILE...` — list common file data.
- `lf -g FILE...` — show generic ratings.
- `lf -s N FILE...` — show Sense layer N.
- `lf -H FILE...` — show health.
- `lf -a FILE...` — show supported metadata.
- `lf --schema` — show schema.
- `mf ...` — modify selected metadata fields.
- `comb` — collect/validate metadata.
- `drm` — plan or perform authorized removal of Ubuntu White metadata according to its safety contract.
