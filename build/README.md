# MirvkBuntu Build System

Two development targets are provided:

- `build-minimal.sh`: minimum bootable development image with networking, SSH, Git, compiler tools, and Linux.
- `build-desktop.sh`: minimum GNOME desktop development image.
- `build-iso.sh`: convenience entry point for the desktop ISO.

The scripts use Debian Live's `live-build` to create ISO-hybrid images. Generated workspaces and ISO artifacts stay under `build/work/` and `build/output/`.

On Ubuntu/Debian hosts:

```bash
sudo apt update
sudo apt install live-build debootstrap xorriso squashfs-tools mtools dosfstools
```

Build from the repository root:

```bash
./build/build-minimal.sh
./build/build-desktop.sh
```

The desktop image is the initial boot-to-desktop development copy. MirvkBuntu-specific packages, kernel configuration, branding, and first-boot provisioning can be layered onto these reproducible entry points later.
