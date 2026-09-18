# Aptitude — Context-Aware Installer

Aptitude is an experimental installation layer for Linux that turns a dropped software artifact into a **reviewable, verifiable installation plan**.

The project's informal phrase **sentient install** means *situationally aware installation*: Aptitude observes the host, identifies available integration surfaces, constructs a plan, asks for authorization when required, executes only the approved operations, verifies the result, and records what happened. It does not claim consciousness.

## Design

```text
artifact
   ↓
identify → inspect → verify
   ↓
host discovery
   ↓
plan
   ↓
review / authorization
   ↓
apply
   ↓
verify → evidence
```

## Initial Linux surfaces

The first implementation recognizes and reports:

- Linux architecture and kernel identity
- ELF executable identity and dynamic dependencies
- filesystem installation targets
- PATH and JAVA_HOME
- systemd/systemctl when available
- cron/crond when available
- system/user timers when available
- users/groups and file permissions
- package-manager presence
- shared-library availability
- cgroup information when available
- desktop integration surfaces when detectable

Aptitude does **not** assume that every surface exists. Detection produces evidence; evidence produces a plan; the plan determines which adapters can be used.

## Safety model

A binary is never automatically treated as trustworthy merely because it was dropped into the staging directory. Production Aptitude should require, as applicable:

1. artifact identification;
2. architecture/ABI compatibility;
3. cryptographic hash;
4. signature/provenance validation;
5. dependency inspection;
6. explicit installation plan;
7. administrator authorization for privileged changes;
8. post-install verification;
9. rollback where practical.

The initial prototype is intentionally **dry-run by default**.

## Command shape

```text
aptitude plan ./SecureJDK28.sans
aptitude inspect ./SecureJDK28.sans
aptitude apply ./SecureJDK28.sans
aptitude verify SecureJDK28
```

`plan` should be the normal first operation. `apply` is an explicit state-changing operation.

## Secure JDK integration

Aptitude is designed to become the Linux integration layer for Secure JDK 28:

```text
SecureJDK28.sans
      ↓
   Aptitude
      ↓
  host discovery
      ↓
 JAVA_HOME / PATH / libraries
 systemd / timers / permissions
      ↓
 verification
```

This lets the Secure JDK installer remain focused on product configuration while Aptitude handles host integration.

## Total integration

The project can later place the existing Total moderation layer beneath Aptitude:

```text
Aptitude
   ↓
Sentient Plan
   ↓
Total
   ↓
Linux / kernel / hardware
```

Total can supply provenance, resource policy, evidence retention, and controlled service mediation without granting Aptitude unrestricted authority.

## Terminology

**Aptitude** — the product/project name for the context-aware installation layer.

**Sentient install** — an informal product term meaning host-aware, evidence-driven installation. It does not assert machine consciousness.

**Surface** — a detectable operating-system integration mechanism such as systemd, cron, PATH, shared libraries, users/groups, or desktop registration.

**Plan** — the proposed set of installation operations and their evidence before state-changing execution.

**Aperture** — the amount of configuration and host integration exposed to the operator.

## Current boundary

The prototype should inspect and plan first. It should not silently install arbitrary dropped binaries, create persistent services, modify privileged configuration, establish network listeners, or schedule recurring execution without explicit authorization.

That boundary is part of the product: Aptitude is intended to make installation **smooth without making it opaque**.
