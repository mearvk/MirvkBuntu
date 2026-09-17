# MEARVK SecureJDK 28 — ASYSMA Synthesis Bridge

## Status

**Design / integration bridge**

This document defines the synthesis boundary between the MEARVK ASYSMA
native development and the OpenJDK 28 source tree.

The current target is **SecureJDK 28**.

Graal is not part of this integration path.

---

## Purpose

The ASYSMA native system provides a small native bootstrap and host
observation layer which can precede Java execution.

SecureJDK 28 remains responsible for Java execution and the Java runtime.

The bridge establishes the following progression:

```text
Desktop OS already running
        |
        v
x86-64 native bootstrap
        |
        v
Ubuntu White Direct
        |
        +-- CPU / architecture evidence
        +-- OS family
        +-- OS version
        +-- memory
        +-- storage
        |
        v
ASYSMA verification / policy
        |
        v
SecureJDK 28
        |
        v
Java executor
        |
        v
JavaFX / application
