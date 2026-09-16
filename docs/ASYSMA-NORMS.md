# ASYSMA Norms and Proceed — 1-2-3-4

**Status:** Normative engineering record
**Date:** 2026-08-26
**Scope:** ASYSMA, XMC, native bootstrap, host/integrity layer, execution bridge, Java/application layer

## Norms

### N1 — Correctness precedes composition

The compiler must establish source and output correctness before packaging, registration, desktop integration, or execution composition occurs.

### N2 — One authoritative version

Every executable component and document must obtain its XMC/ASYSMA format version from an authoritative definition. Duplicated version literals are prohibited when they can diverge.

### N3 — Bounded parsing is a refusal boundary

Resource limits are security boundaries. Reaching a limit, encountering malformed syntax, overflowing a representation, or observing an invalid index must produce an explicit diagnostic and controlled failure. It must not be interpreted as successful compilation.

### N4 — Facts are distinct from classifications

Compiler facts, parser results, security observations, provenance, and optional project classifications are separate data classes. A classification cannot override a correctness or security failure.

### N5 — Digest is not signature

SHA-256 is an integrity digest. It is not an authentication signature. XMC must use the terms **digest**, **signature**, **provenance**, and **authorization** precisely.

### N6 — No shell interpretation in trusted paths

Compiler-driver and registration paths must not construct shell commands from untrusted paths or metadata. Child programs use explicit argument vectors; filesystem operations use native APIs where practical.

### N7 — Output is atomic and deterministic

Generated artifacts must be deterministic for a fixed source/toolchain/input profile, and writes must use a temporary-file plus atomic rename strategy where the platform supports it.

### N8 — Observation does not authorize execution

Filesystem metadata, CTRMS observations, filenames, hashes, executable identity, and package provenance are evidence. None independently grants execution authority.

### N9 — Native capability is explicit

A native payload or native execution mode must be explicitly represented in the package contract. A Java class, filename, or textual pattern must not silently acquire native execution capability.

### N10 — User-scoped registration

Desktop/MIME integration must remain user-scoped unless the installation contract explicitly requests a system-wide operation. XMC must not silently escalate privileges.

### N11 — Platform boundaries are explicit

ELF, PE/COFF, and Mach-O loading remain operating-system responsibilities. ASYSMA provides a package and handoff contract rather than a replacement operating system.

### N12 — Tests define the contract

Every security boundary and public compiler behavior must have a regression test. Strict compilation, smoke tests, hostile-path tests, deterministic-output tests, and sanitizer tests are release gates.

## Proceed — ordered engineering sequence

1. **Core correctness:** remove duplicate version authority; validate numeric input; audit bounds, indexing, string termination, file reads, and parser failure states.
2. **Parser integrity:** harden Java, Python, and Rust structural scanning against strings, comments, nesting, malformed syntax, and resource exhaustion.
3. **Output correctness:** make `.xclass` deterministic; escape serialized values; validate output; write atomically.
4. **Metadata isolation:** move quality/project classifications behind an explicit metadata boundary.
5. **Cryptographic semantics:** retain SHA-256 as a digest and design a separate authenticated-signature envelope for releases.
6. **Tests:** add language, malformed-input, limit, path, reproducibility, serialization, digest, and integration regression suites.
7. **Sanitizers/fuzzing:** run ASan/UBSan and add bounded parser fuzz/property tests.
8. **ASYSMA packer/bootstrap:** verify binary layout, bounds checking, entry modes, and native-to-Java handoff.
9. **OS integration:** complete independent Linux/macOS/Windows registration implementations and verify least-privilege behavior.
10. **Release gate:** publish only after strict compilation and all applicable tests pass on the target platforms.

## 1-2-3-4 contract

```text
1 Native Foundation
        ↓
2 Host + Integrity
        ↓
3 Execution Bridge
        ↓
4 Java / Application
```

Each stage consumes validated output from the previous stage. A later stage cannot convert a failed earlier stage into success.

## Evidence model

```text
Observation → normalized fact → provenance → integrity evidence → authorization policy → execution decision
```

These are intentionally separate. In particular, a hash proves correspondence to a known digest; it does not prove who produced an artifact or whether the current actor is authorized to execute it.

## XMC-specific quality gate

XMC is considered ready to advance from core correction to ASYSMA composition only when:

- `--version` and compiler metadata agree;
- strict C compilation succeeds without unexplained diagnostics;
- malformed input fails explicitly;
- resource limits fail explicitly;
- hostile filesystem paths are treated as data;
- generated output is deterministic under the defined profile;
- SHA-256 tests pass;
- ASan/UBSan tests pass;
- no trusted driver path depends on shell interpretation.

## Change discipline

The 1, 2, 3, 4 documents and the Mermaid design record are normative documentation companions to the implementation. When implementation behavior changes, the applicable numbered document and the `.mmd` record must be updated in the same engineering change or the discrepancy must be explicitly recorded.
