# MirvkBuntu Installers

MirvkBuntu has a native cross-platform installer target under installer/.

The installer is separate from the ISO builder:
- Linux: native ELF installer plus Debian/TGZ packages.
- Windows 10+: native Windows executable plus NSIS installer/ZIP.
- macOS: native Mach-O installer plus product installer/DMG.

CMake provides the common build model and CPack provides platform-specific packaging. CMake documents CPack as a cross-platform packaging system for Linux, Windows, and Mac.

## Local build

Linux:
    cmake -S installer -B build/installer -DCMAKE_BUILD_TYPE=Release
    cmake --build build/installer --config Release
    cpack --config build/installer/CPackConfig.cmake

Windows 10+:
    cmake -S installer -B build/installer -A x64
    cmake --build build/installer --config Release
    cpack --config build/installer/CPackConfig.cmake -C Release

macOS:
    cmake -S installer -B build/installer -DCMAKE_BUILD_TYPE=Release
    cmake --build build/installer --config Release
    cpack --config build/installer/CPackConfig.cmake

## CI artifacts

GitHub Actions builds:
- MirvkBuntu-Installer-Linux-x86_64
- MirvkBuntu-Installer-Windows-x86_64
- MirvkBuntu-Installer-macOS-universal

The macOS workflow builds arm64 and x86_64 together. Public macOS distribution should use Developer ID signing and notarization. Apple documents notarization for apps, installer packages, and disk images distributed outside the App Store.

The installer is intentionally conservative. It does not repartition disks, replace boot loaders, or silently overwrite an existing operating system. Those actions belong to an explicit OS installation workflow.
