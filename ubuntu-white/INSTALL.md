# Ubuntu White installation plan

This is a conservative installation specification for the theme. It intentionally follows the repository's Aptitude model: inspect → plan → authorize → apply → verify. Aptitude's current documentation identifies desktop integration as a host surface and states that persistent changes should not happen silently. fileciteturn20file0

## Planned targets

- GTK theme CSS
- SVG icon theme assets
- desktop font preference
- optional shell/window-manager integration where the host exposes a supported API

## Detection

Before applying, detect:

- desktop/session type;
- GTK version;
- current theme and icon theme;
- supported theme directories;
- writable user-level theme locations;
- current font configuration;
- whether the session can reload the theme without logout.

## Default policy

Prefer user-local installation over system-wide installation. Do not overwrite an existing theme. Do not modify privileged files without explicit authorization. Record the prior theme configuration so rollback is possible.

## Desired visual result

White workspace and folder surfaces, dark-grey GUI controls and text, named navigation items, restrained shadows, a cool modern sans-serif font, strong focus visibility, and comfortable spacing.

## Future installer command shape

```text
aptitude inspect ubuntu-white
aptitude plan ubuntu-white
aptitude apply ubuntu-white
aptitude verify ubuntu-white
```

`apply` remains an explicit state-changing operation.
