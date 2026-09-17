# XML Class File Format — Galactic Cherry Marvell Edition 98

## Overview

The XML Class File Format (`.xclass`) is a human-readable, richer alternative to the
standard Java `.class` binary format (`0xCAFEBABE`). It carries all information present
in a standard class file plus additional metadata that the binary format cannot express.

## File Extension

`.xclass` — XML Class File

## Magic Identifier

The XML class file is detected by the XML processing instruction:
```xml
<?xclass version="1.0"?>
```

Or by the root element:
```xml
<class xmlns="urn:galactic-cherry:xclass:1.0">
```

## Advantages Over Binary .class

| Feature | Binary .class | XML .xclass |
|---------|--------------|-------------|
| Human-readable | No | Yes |
| Inline documentation | No (only via attributes) | Full XML comments + `<doc>` elements |
| Design intent | Not expressible | `<intent>`, `<contract>`, `<invariant>` |
| Provenance | No | `<author>`, `<origin>`, `<license>` |
| Security metadata | Limited | `<security>` block (permissions, trust grade) |
| Version history | Not expressible | `<history>` with diffs |
| Dependencies declared | Implicit (constant pool refs) | Explicit `<dependencies>` |
| Build requirements | Not expressible | `<build>` block |
| Optimization hints | Limited (StackMapTable) | `<hints>` for JIT/AOT |
| Testability | Not expressible | Inline `<test>` specifications |
| Integrity | None | SHA-256 signature over content |

## Format

See xmlClassFormat_example.xclass for a complete example.
