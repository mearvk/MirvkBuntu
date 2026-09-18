# Negamane Platform Model

## Scope

Negamane is the integrity and provenance boundary for Ubuntu White filesystem monitoring. It supports both OS-derived implementations:

1. **Linux/Ubuntu White** — kernel/filesystem interface with EXT4 compatibility and Linux device-health sources.
2. **Windows-derived implementation** — user/kernel boundary using Windows storage and file-integrity facilities, including SMART/reliability evidence where available.

The two implementations share the same logical schema; platform-specific evidence and mechanisms remain native to each OS.

## Three-copy model

Negamane understands the Ubuntu White three-copy model. A logical file may have Copy/Sense 1, 2, and 3. Each copy independently carries the generic rating set, health state, content identity, device evidence, and alteration history. Negamane protects the identity and integrity of the copy set and its monitoring records.

## Base filesystem mode

Ubuntu White MAY be installed in **Base Filesystem Mode** when disk-space conservation is preferred. In Base Filesystem Mode there is one ordinary file copy and the standard filesystem metadata model; Ubuntu White three-copy records are not required.

This is an explicit installation/profile choice, not an automatic degradation. Tools must report the selected mode and must never claim three-copy redundancy when Base Filesystem Mode is active.

## Generic schema

The shared schema contains the canonical 18 generic ratings and per-copy overall health. Negamane protects the schema definition, version, and policy artifacts so that a mutable local process cannot silently redefine their meaning.

## Monitor protection

Filesystem monitors and policy executables should be verified against a trusted reference before execution and after updates. Negamane can use platform-native code signing, trusted boot/secure-boot facilities, immutable or protected deployment paths, hashes/signatures, and recovery copies as available. Negamane itself must not rely solely on the mutable metadata store that it is auditing.

## Compatibility principle

Platform-specific implementation details MUST NOT leak into the shared logical model. Linux may use EXT4/xattrs and Linux storage interfaces; Windows may use NTFS/ReFS and Windows storage reliability interfaces. The common records describe the same concepts while preserving source provenance.
