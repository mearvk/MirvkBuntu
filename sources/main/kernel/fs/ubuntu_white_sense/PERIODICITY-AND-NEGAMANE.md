# Ubuntu White Holistic Periodicity and Negamane Monitor Integrity

## Purpose

Ubuntu White alteration history uses two related retention regimes: holistic periodic sampling for depths 1–8 and epoch-based retention for depths 9–24.

## Holistic periodicity: depths 1–8

Depths 1 through 8 are the **holistic observation band**. The filesystem monitor considers the whole managed copy set rather than only the latest alteration.

A configured periodicity determines when the monitor creates a holistic checkpoint. The period is expressed as a relative duration and MUST be stored with the checkpoint so the schedule is reproducible. The implementation may select an appropriate period for workload and storage capacity; it must not pretend that an unspecified period is fixed.

A depth-8 policy may use a retention target such as 3,000 active alteration records. This is a policy example, not a kernel constant.

## Epoch periodicity: depths 9–24

Depths 9 through 24 are retained by **epoch**. An epoch is a configured, consistently measured relative modification interval. The epoch length should normally remain stable for a given policy profile.

Each epoch records its boundary, policy version, and the first/last alteration identifiers it contains. Epoch summaries can be moved to an archival tier while preserving chain identity.

This produces a bounded hierarchy:

`depth 1–8 = holistic periodic checkpoints`

`depth 9–24 = epoch-based historical aggregation`

## Aging and archival

When active history exceeds its configured retention boundary, old records may fall out of the local active window only after they have been durably handed to an explicitly configured archival tier. The archive remains provenance-linked through record IDs/digests.

No age policy may silently rewrite or destroy current file contents.

## Negamane

**Negamane** is the project name for the integrity-anchor mechanism protecting filesystem-monitor programs and their configuration from unauthorized modification.

Negamane is a protection/integrity concept, not a claim that software can be mathematically impossible to modify. A production implementation should use standard OS controls, signed artifacts, immutable/read-only deployment where appropriate, measured boot or trusted boot facilities where available, and cryptographic integrity verification.

The monitor should verify its own executable, configuration, schema, and policy inputs before trusting them. A mismatch is an integrity event and must be reported rather than silently repaired.

## Monitor separation

`comb`, `lf`, `mf`, and future filesystem monitors should have independently verifiable identities. The monitoring path must not depend on the same mutable metadata it is responsible for auditing.

Where practical:

1. monitor binaries are installed from signed/trusted artifacts;
2. policy/configuration is integrity-checked;
3. monitor results identify the monitor version and policy version;
4. alteration history records the monitor identity;
5. monitor self-modification is prohibited by policy;
6. recovery uses a known-good artifact rather than an automatically modified executable.

## Reference

This document is normative for the Ubuntu White alteration-history prototype and is referenced by the filesystem vocabulary, alteration-history specification, and COMB monitor design.
