# ICC — Legal Standings documents

**Project:** Ubuntu Determinant
**Edition:** Ubuntu White Edition
**Project attention:** Max Rupplin — MEARVK LLC — 2026
**Status:** Complete — original `.docx` present, integrity-hashed; signature/timestamp procedures documented (pending external run)

---

## Purpose

This folder holds the ICC Legal Standings documents and their integrity,
verification, binding, signing, and reference materials.

## Contents

| File | What it is |
|---|---|
| `ICC Legal Standings 1.docx` | Original document 1 (OOXML `.docx`, validated intact) |
| `ICC Legal Standings 2.docx` | Original document 2 (OOXML `.docx`, validated intact) |
| `ICC Legal Standing 3.docx`  | Original document 3 (OOXML `.docx`, validated intact) |
| `ABOUT-THE-COURT.md` | Reference note on the ICC, keyed to icc-cpi.int/about/the-court |
| `SHA256SUMS` | SHA-256 digests of every file (verify with `sha256sum -c`) |
| `INTEGRITY.md` | Integrity record + timestamping options/standards (RFC 3161, eIDAS QTS, OpenTimestamps, ERS series) |
| `CERTIFICATION-ICC-DISNEY.md` | Cross-document binding (SHA-256) tying these docs to the Disney public-key document |
| `SIGNING.md` | OpenSSL procedure to sign the binding with a project (MEARVK) key |

## Integrity

The three `.docx` were validated as intact OOXML (all internal ZIP CRC-32 checks
pass, `word/document.xml` present) and hashed. Verify anytime:

```sh
cd ICC
sha256sum -c SHA256SUMS      # each file -> OK
```

Digests, timestamping options, and the ICC↔Disney binding hash are recorded in
`INTEGRITY.md` and `CERTIFICATION-ICC-DISNEY.md`.

## Pending (external, honest status)

Two items require a machine with internet / `openssl` (both blocked in the
authoring environment; nothing was fabricated to fill them):

1. **Trusted timestamp** over the documents/binding — see `INTEGRITY.md` §4–§5
   (RFC 3161 TSA, eIDAS Qualified Timestamp, or OpenTimestamps). Commit the
   returned token and fill `INTEGRITY.md` §6.
2. **Project signature** over the binding — see `SIGNING.md`. Commit the public
   key + `binding.sig.b64` and fill `CERTIFICATION-ICC-DISNEY.md` §6.

## Note

Placement in this repository does not assert any official ICC status or
endorsement, nor any relationship with The Walt Disney Company. Materials here
are supplied by the requester for storage, integrity, and reference.
