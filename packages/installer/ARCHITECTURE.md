# White Edition Installer Architecture

## 1. Layers

```text
JavaFX Professional Launcher
        |
        +-- Discovery
        +-- Plan / Review
        +-- Verification
        |
        +-- Linux Adapter ------> existing Make/ISO contracts
        +-- Windows Adapter ----> WSL/QEMU integration
        +-- VM Adapter ---------> QEMU / platform VM
        |
        +-- Privileged Helper -- only for explicitly confirmed operations
```

The console/native orchestrator `white-installer`
(`installer/linux/white_installer_orchestrator.c`) is the C-side control-plane
sibling to the JavaFX launcher. It drives the existing Make/ISO/installer
contracts (notably the Bash engine `scripts/galactic-cherry-installer`) rather
than duplicating them, keeps the display and probe unprivileged, and emits the
section-7 audit report. See `installer/INSTALL.md` for its flow, flags, and
delegation scheme.

## 2. Existing repository integration

The repository already has an ISO build target and a `scripts/gen-iso.sh` implementation. The new interface should call those established contracts rather than create a competing ISO implementation. fileciteturn195file0L2-L2 fileciteturn200file0L2-L2

The repository also has an existing Ubuntu graphical-installer integration through `scripts/install-ubuntu-installer.sh`. The White Edition interface is therefore a control plane and launcher, not an attempt to duplicate Subiquity itself. fileciteturn199file0L2-L3

## 3. Root directory installation

A root-directory installation is useful for:

- image assembly;
- chroot development;
- testing;
- alternate rootfs locations;
- VM disk preparation.

It should use the existing rootfs assembly and installation contracts and must not overwrite a non-empty directory without an explicit review step.

## 4. Named partition installation

Partition installation is the highest-risk local operation. The UI must display:

- device path;
- partition number/name;
- capacity;
- filesystem;
- current mount points;
- whether the device is removable;
- intended formatting operation;
- intended mount point;
- bootloader target;
- data-loss warning.

The first release should require an external privileged helper for actual partition writes. The JavaFX process must not run as root merely to display the installer.

## 5. Existing ISO / VM

Running an existing ISO is read-only with respect to the host when executed in an isolated VM. QEMU is the initial cross-platform VM target because it is available on Linux and Windows installations and provides a common invocation model.

The VM path should eventually support:

- ISO selection;
- RAM/CPU configuration;
- UEFI/BIOS selection;
- virtual disk creation;
- persistent VM state;
- disposable test VM;
- networking mode;
- secure boot policy where supported.

## 6. Windows

Windows should not pretend to provide Linux-native disk tooling. The installer uses a Windows adapter and can use WSL2 for Linux-side build work and QEMU for VM execution. Native Windows disk operations require a separate, explicit administrative integration layer.

## 7. Audit

Each completed operation should produce a small report:

```text
installer version
host
operation
source
resolved target
commands/contracts invoked
privilege boundary
start/end time
verification result
warnings
```

No passwords, private keys, or unrelated personal information belong in the report.

## 8. White Edition presentation

The professional interface uses a restrained white surface and makes the state of the system visible. The installer should be clean and business-friendly rather than visually aggressive.
