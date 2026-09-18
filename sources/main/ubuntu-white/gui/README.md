# Ubuntu White Desktop Development

## Developer compile method

The Ubuntu White desktop source is intended to be buildable locally without requiring an OS installation. This directory is the developer-facing GUI workspace.

### Linux

From the repository root:

```sh
cd main/ubuntu-white/gui
./build-linux.sh
```

The script should compile the desktop from the source currently present in this directory and place build products under `build/`. It must not modify source files.

### Look-and-feel development

Developers may iterate on layout, typography, spacing, controls, icons, themes, and accessibility while keeping the build procedure stable. Build configuration belongs in `build/` or generated files and must not be mixed with source assets.

### Scope

This is a development build method, not the production OS installer. Production packaging and signing remain separate steps.
