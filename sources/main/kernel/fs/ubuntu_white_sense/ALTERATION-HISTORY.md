# Ubuntu White Alteration History and Required Depth

## Status

Core filesystem design requirement.

Every managed file copy MUST have an append-oriented alteration history. The history records changes over time without replacing prior records. A linked-list or equivalent authenticated append chain is the reference model.

## Required alteration record

Each record contains at least:

- sequence number;
- previous-record identifier/hash;
- timestamp;
- file/copy identity;
- alteration type;
- actor/tool identity;
- authorization/reference identifier;
- before content identity;
- after content identity;
- alteration depth;
- schema version.

## Required depth

Every alteration MUST declare a non-negative integer `alteration_depth`.

- `0` = creation/initialization or no alteration ancestry.
- `1` = direct alteration of the current file copy.
- `2+` = an alteration derived from a previously altered state; the value records the declared ancestry depth.

Depth is descriptive provenance, not an authority score and not a measure of a person's status.

## Integrity

The preferred implementation is a singly linked append chain:

`record[n].previous_id = record[n-1].id`

For stronger integrity, each record may additionally carry a cryptographic digest over its canonical serialized fields and the previous digest.

A missing predecessor, invalid sequence, timestamp regression where prohibited, or broken digest must be reported by `comb` as a history-integrity error.

## File replacement

Replacing a physical copy creates a new history record. The previous copy state remains part of the historical record. The implementation must not silently erase provenance.

## Deletion

`drm` must record an authorized deletion event before removing a managed copy when a writable history store is available. Destruction of the history itself is a separate operation and requires explicit authorization.

## Kernel boundary

The history contract belongs to the Ubuntu White filesystem model. The initial implementation may use a journal/sidecar store while EXT4 compatibility is preserved. Native kernel persistence must be atomic with the corresponding filesystem operation before being treated as production-ready.
