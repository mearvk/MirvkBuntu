# Linux Desktop Install Probe

A small native C helper for the Step 1 desktop-preview/install workflow.

## What it does

The helper follows the installation order:

**Git clone → install set → desktop preview → dry run → explicit install**

It looks for an existing repository first. If none is found, it uses:

`$HOME/src/Ubuntu.Determinant.Beta.Restricted`

and performs a shallow Git clone.

It then looks for either:

- `installer/install-manifest.txt`
- `installer/install-native.sh`

and reports the Step 1 desktop-preview document.

## Build

```sh
make
```

## Install locally

```sh
make install
```

The default destination is:

`$HOME/.local/bin/desktop_install_probe`

## Run discovery

```sh
desktop_install_probe
```

Discovery is intentionally a dry run and performs no installation.

## Run installation

```sh
desktop_install_probe --install
```

This explicitly invokes `installer/install-native.sh` when that script exists.

## Design intent

The helper is deliberately modest: it is a bootstrap point, not the complete installer. The desktop preview and installation profile remain separate design artifacts so that the user can inspect the intended environment before committing to installation.
