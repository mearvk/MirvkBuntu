# MirvkBuntu Installer Menu

The installer presents the same simple top-level workflow on Linux, Windows 10+, and macOS while routing each operation to its native platform backend.

1. Hardware & system check
2. Storage & partitions
3. Language & keyboard
4. Time zone & clock
5. Network & hostname
6. User accounts
7. MirvkBuntu payload
8. Bootloader
9. Installation plan
10. Install / Continue
0. Exit

The menu is intentionally simple. Platform-specific implementation remains behind the installer interfaces, so the Windows and macOS front ends do not pretend to be Linux storage tools.

Installation is not destructive merely because a menu item is selected. Disk mutation requires an explicit installation plan and confirmation.