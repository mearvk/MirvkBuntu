# ClamAV — Malware Detection and Source-Tree Screening

**Project role:** malware detection and defensive scanning of source trees, packages, build artifacts, and other files.

ClamAV is an open-source antivirus toolkit that can scan files and streams using signature databases and additional detection mechanisms. In this OS project it can provide a defensive inspection layer around source acquisition, package intake, staging areas, and release artifacts.

## Safety design

- Scan newly acquired source archives and extracted trees before compilation or installation.
- Keep ClamAV databases current and record the database/version used for an important scan.
- Prefer scanning in an isolated, unprivileged workspace before files enter trusted build or system locations.
- Preserve scan logs and associate them with the source commit, archive digest, or package digest being examined.
- Combine scanning with SHA-256 integrity records, trusted Git provenance/signatures, source-tree validation, and dependency review.
- Treat archives and unusual file formats as untrusted input and avoid automatically executing anything discovered during scanning.
- Use a staged build process so a detected file can be quarantined without modifying the trusted source tree.

## Limitations

ClamAV is **not proof that software is safe**. A clean scan means that the configured detection engine and databases did not identify known or detectable malware; it does not establish absence of vulnerabilities, backdoors, malicious build logic, supply-chain compromise, or previously unknown malware.

Signature databases can lag behind newly discovered threats. Detection can also vary with file format, configuration, database version, archive handling, and resource limits. False positives and false negatives are possible.

ClamAV does not replace source-code review, compiler/toolchain hardening, dependency verification, cryptographic provenance, sandboxing, or operating-system access controls. It should not be treated as a security boundary for hostile programs.

## OS integration policy

Use ClamAV as one layer in a defense-in-depth acquisition pipeline:

1. acquire into an isolated temporary directory;
2. verify upstream provenance and expected revision;
3. calculate and record SHA-256 digests;
4. validate the source-tree structure and reject unexpected install paths or executable hooks;
5. scan the complete acquisition with ClamAV;
6. compile without elevated privileges;
7. stage installation under a controlled prefix;
8. inspect the staged manifest before any privileged installation.

A clean ClamAV result must never be interpreted as permission to execute untrusted code or install it directly into the operating system.

**Optimized designation:** Max Rupplin — MEARVK LLC — 2026.
