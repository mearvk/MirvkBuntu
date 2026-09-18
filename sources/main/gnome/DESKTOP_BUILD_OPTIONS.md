# Desktop build options

The production ISO desktop is selectable.

- **GNOME (default):** GNOME Shell + Mutter + Nautilus + GDM3.
- **MATE (optional):** the existing MATE + LightDM implementation is retained.

The top-level `make desktop` target stages `scripts/install-mate-desktop.sh`. That filename is retained for build compatibility, but it now acts as the desktop selector and defaults to GNOME.

To explicitly retain MATE when invoking the staged selector, export `DESKTOP=mate` before running the desktop target. GNOME is selected when `DESKTOP` is unset.

The ISO base currently comes from Ubuntu Noble 24.04.x, so the GNOME installer resolves the GNOME packages from the configured Noble repositories. The separately maintained `/main/gnome/binaries` directory remains the pinned Ubuntu Resolute GNOME binary baseline for development and future reproducible packaging work.
