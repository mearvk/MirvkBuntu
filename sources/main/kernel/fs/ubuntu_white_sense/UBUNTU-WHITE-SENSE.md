# Ubuntu White Sense, COMB, LF, MF, and DRM Filesystem Metadata

## Vocabulary

See [`VOCABULARY.md`](./VOCABULARY.md) for the canonical definitions of Ubuntu White, Sense, COMB, `lf`, `mf`, DRM, Hold type, file payload, and the 18 generic rating names. The vocabulary is normative for this prototype. fileciteturn69file0

## Purpose

Ubuntu White adds a filesystem-level metadata model for installed files. Ordinary file contents remain ordinary file contents. A bounded metadata record describes provenance, lifecycle, generic classification, and up to three deliberate Sense observations/copies.

## Sense layers

Each managed file has up to three ordered Sense layers. When all three are present, they are Sense 1, Sense 2, and Sense 3. **Each Sense layer has its own 18 generic ratings and its own overall health rating.** Ratings are metadata, not executable policy and not intrinsic judgments about people.

## 18 generic ratings

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

Each rating has a generic, schema-defined value representation. Implementations must not infer sensitive human attributes from filenames, ratings, or Sense data.

## Overall health

Each Sense layer contains an `overall_health` value. Health is an implementation/reporting measure for metadata/file integrity and lifecycle state; it is not a human worth, intelligence, social status, or authority score.

## Hold type

Each managed file record may carry a `hold_type` string. It is an extensible generic classification with no authority by itself. Unknown values remain readable and must not cause destructive behavior.

## COMB

`comb` is the system collector for this metadata. It provides a consistent inventory view, validates records, reports malformed or duplicate metadata, and is read-mostly by default. It must not silently rewrite file contents or grant privilege.

## LF

`lf` is the Ubuntu White companion to `ls`. It lists common file data and can optionally display Sense-layer ratings and health.

## MF

`mf` modifies supported file metadata, including identity/name/date/author/database and Sense metadata. It must not alter the file payload merely to change metadata.

## DRM

DRM is the metadata-removal interface. Its vocabulary and scope are limited to the Ubuntu White metadata class; it must not treat metadata removal as permission to destroy ordinary file contents or EXT4 structures.

## Filesystem boundary

The prototype remains compatible with stock EXT4 by using a sidecar/index representation. It does **not** change EXT4's native on-disk format in this stage. Native xattr/inode integration requires a separate compatibility, recovery, fsck, and upstream-kernel review.

## Installation behavior

The OS installer creates fresh metadata records for files it installs. Existing records are preserved unless an explicit migration is requested. `comb` and `lf` do not modify file payloads merely to inspect metadata.
