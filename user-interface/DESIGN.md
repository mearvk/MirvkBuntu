# User Interface Design

## Best-of-breed reference analysis

### Chromium

Chromium's open-source tree demonstrates a mature split between browser/application UI, WebUI surfaces, command controllers, resources, and platform-specific implementations. The UI is integrated with the browser's service architecture rather than treated as an isolated skin.

Reference: https://github.com/chromium/chromium/tree/main/chrome/browser/ui

### Brave

Brave Core demonstrates a useful model for product differentiation without replacing the entire Chromium architecture: product-specific UI, commands, WebUI, icons, and resources are layered around a common browser foundation. Brave documents a preference for override/patch mechanisms that preserve the underlying Chromium implementation where practical.

Reference: https://github.com/brave/brave-core/tree/master/browser/ui

### Firefox

Firefox provides a strong reference for desktop front-end organization, themes, reusable UI components, and design-system documentation. Its browser front-end documentation explicitly separates front-end concerns from the rest of the browser system.

Reference: https://github.com/mozilla-firefox/firefox/tree/main/browser

## White Edition adaptation

The project should adopt the **architecture patterns**, not copy the products:

```text
Reference architecture
        ↓
interaction pattern
        ↓
White Edition component
        ↓
JavaFX implementation
        ↓
Host/System Contract
```

### Primary surfaces

| Surface | Purpose |
|---|---|
| Shell | Identity, navigation, window state |
| Host | OS, architecture, resources |
| Kernel | Kernel identity and native capability boundary |
| Stability | Health and compatibility evidence |
| Security | Privilege, authorization, audit |
| AI | Observable runtime/accelerator capability |
| ISO | Discover, build, verify |
| Install | Root, partition, recovery |
| VM | Safe test execution |
| Report | Evidence and final result |

### Visual language

The White Edition should use:

- white primary surfaces;
- restrained neutral borders;
- compact professional controls;
- strong typographic hierarchy;
- modest status indicators;
- high-contrast focus states;
- consistent spacing;
- minimal decoration;
- no copied browser branding.

### Interaction model

```text
Discover → Assess → Plan → Review → Authorize → Execute → Verify → Report
```

Destructive operations never jump directly from discovery to execution.

## Licensing and provenance

The references above are external projects with their own licenses and attribution requirements. This repository should preserve those distinctions. No Chromium, Brave, or Firefox source is being represented here as MEARVK-authored code merely because its interaction model informed this design.

**Project attention:** Max Rupplin — MEARVK LLC — 2026.
