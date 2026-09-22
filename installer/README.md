# MirvkBuntu Installer

MirvkBuntu now has a cross-platform installer architecture for Linux, Windows 10+, and macOS.

## Installer facets

- Hardware/platform detection.
- Disk discovery and target validation.
- Explicit GPT/UEFI partition planning.
- Filesystem creation and mounting interfaces.
- MirvkBuntu payload, kernel and initramfs installation interfaces.
- Bootloader installation interface.
- Language/locale selection.
- Keyboard-layout selection.
- IANA timezone selection.
- Clock/time synchronization configuration.
- Hostname and network configuration.
- User/account and administrator-group configuration.
- Package/runtime installation interfaces.
- Desktop and first-boot configuration hooks.
- Verification and reboot hand-off.
- Dry-run and preflight modes.

## Platform separation

### Linux

Linux is the native MirvkBuntu installation target. The installer can integrate with standard Linux utilities such as lsblk, mkfs, localectl, and timedatectl. Bootloader, filesystem, account, network, and payload operations are exposed as explicit installer stages.

### Windows 10+

The Windows executable provides a native installer front end and platform backend boundary for Win32/PowerShell storage, locale, account, networking, and payload operations. It must never silently erase Windows or repartition a disk.

### macOS

The macOS executable provides a native installer front end and platform backend boundary for diskutil, system configuration, locale, timezone, account, networking, and payload operations. It must not silently modify APFS volumes or firmware settings.

A Windows or macOS host executable is not pretending to be a Linux disk installer. MirvkBuntu Linux installation is performed from the Linux installation environment.

## Safety model

The default is dry-run/planning mode. Disk-affecting execution requires:

1. An explicit target.
2. A complete partition/filesystem plan.
3. Administrator/root privileges where required.
4. An explicit --confirm.
5. A final plan displayed before mutation.

The current storage and bootloader implementations intentionally stop at discovery, validation, and orchestration boundaries until a concrete MirvkBuntu disk-image/layout contract is supplied. This prevents accidental destructive partitioning while the ISO/build artifacts are being finalized.

## Example

mirvkbuntu-installer --check

mirvkbuntu-installer --plan --target-disk /dev/nvme0n1 --language en_US.UTF-8 --keyboard us --timezone America/New_York

mirvkbuntu-installer --execute --confirm --target-disk /dev/nvme0n1

The final OS image remains produced by the MirvkBuntu build system; the installer consumes that verified payload rather than replacing MirvkBuntu source with generic live-build content.
