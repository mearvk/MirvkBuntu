# Ubuntu Grand — Base Install / Software Release

## Release designation

**Product:** Ubuntu Grand

**Release role:** Base Install

**Iteration:** Current Software Release

**Desktop foundation:** `ubuntu-white`

Ubuntu Grand treats the `ubuntu-white` desktop as the baseline presentation and ergonomic layer for this software iteration. It is the reference desktop configuration against which installation, application integration, accessibility, and system-facing user experience should be evaluated.

## Base-install principle

The base install is intended to be coherent rather than merely minimal. A component belongs in the base when it provides a stable foundation for ordinary operation, system administration, application launch, desktop navigation, security/provenance observation, or supported interoperability.

The base should remain:

- predictable;
- reviewable;
- reversible where practical;
- accessible;
- evidence-driven;
- conservative about privileged changes;
- compatible with the project's Windows/Wine interoperability work where appropriate.

## Desktop baseline

The default visual direction is the `ubuntu-white` specification:

- predominantly white workspace;
- dark-grey controls and window chrome;
- white folder surfaces with restrained dark-grey geometry;
- named and recognizable icons;
- generous spacing and comfortable hit targets;
- modern sans-serif typography;
- visible keyboard focus;
- restrained accent color;
- clear error, disabled, selected, and active states.

The theme specification explicitly describes itself as independent project artwork inspired by the open-source Ubuntu/GNOME desktop experience rather than official Ubuntu branding. See `README.md` and `ubuntu-white.hsss` for the complete design contract.

## Installer relationship

Aptitude remains the preferred integration mechanism. Its existing model is:

```text
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

Ubuntu Grand should use this model for the desktop baseline instead of treating visual configuration as an opaque post-install script.

## Software-release contract

This iteration establishes a release-level expectation that major system components should expose:

1. identity and version;
2. architecture/ABI information;
3. provenance where available;
4. dependencies;
5. supported host surfaces;
6. proposed state changes;
7. authorization requirements;
8. post-install verification;
9. rollback or recovery information where practical.

For Windows interoperability, the same contract applies across native Windows execution, Wine execution, and Linux-native execution. A compatibility layer must report what is translated, bridged, virtualized, rebuilt, or executed directly rather than representing all interoperability as conversion.

## Release quality target

Ubuntu Grand should feel like a single operating environment even when its implementation crosses Linux userland, native components, SecureJDK/Graal, Wine, or Windows-facing compatibility surfaces.

The design goal is **continuity of intent across implementation boundaries**: the user should encounter the same careful naming, evidence, authorization, ergonomics, and recoverability regardless of which subsystem supplies the underlying capability.

## Moral and legal design label

The Ubuntu Grand base-install style is **moral and guided by Law and Morals**. This is a project engineering and design value. It does not create legal authority, alter applicable law, or substitute project terminology for statutory definitions.

## Status

This document establishes the current iteration's intended release baseline. It does not by itself claim that every component has reached production certification; component-level verification remains the responsibility of the relevant build and installation pipeline.
