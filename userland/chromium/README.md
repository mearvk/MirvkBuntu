# Chromium Browser — Open Source

The Chromium web browser source code, fetched at build time via shallow git clone.

## Source

- Repository: https://github.com/chromium/chromium
- License: BSD-3-Clause
- Clone depth: 10 commits (shallow)

## Fetch

```bash
make fetch
# or manually:
./fetch-chromium.sh
```

This performs a `git clone --depth=10` of the official Chromium mirror (~5-8 GB).

## Build (Optional)

Building Chromium from source is resource-intensive:
- **RAM:** 16+ GB
- **Disk:** 100+ GB
- **Time:** 2-4 hours on modern hardware
- **Tools:** depot_tools (gn, autoninja)

```bash
# Install depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:$(pwd)/depot_tools

# Build
make build

# Install
make install DESTDIR=../../build/rootfs
```

## Prebuilt Alternative

For most users, installing the prebuilt package is simpler:

```bash
apt install chromium-browser
```

The `install-mate-desktop.sh` script can be extended to include this if desired.

## Why Include Source?

1. GPL compliance — full source availability for the OS distribution
2. Offline builds — clients can compile without network access
3. Customization — allows building with custom flags or patches
4. Verification — users can audit the browser they're running
