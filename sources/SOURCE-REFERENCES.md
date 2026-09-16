# MirvkBuntu Upstream Source References

This document records the upstream locations used by the initial basic source set.

| Component | Upstream reference | Initial selection |
|---|---|---|
| Linux kernel | https://cdn.kernel.org/pub/linux/kernel/v5.x/ | Linux 5.15.204 |
| Cairo | https://gitlab.gnome.org/GNOME/cairo | default upstream branch at acquisition time |
| GLib | https://gitlab.gnome.org/GNOME/glib | default upstream branch at acquisition time |
| GDK-Pixbuf | https://gitlab.gnome.org/GNOME/gdk-pixbuf | default upstream branch at acquisition time |
| GTK | https://gitlab.gnome.org/GNOME/gtk | default upstream branch at acquisition time |
| Mutter | https://gitlab.gnome.org/GNOME/mutter | default upstream branch at acquisition time |
| GNOME Shell | https://gitlab.gnome.org/GNOME/gnome-shell | default upstream branch at acquisition time |

The GNOME source list follows the module organization already documented in
`Ubuntu.Determinant.Beta.Restricted/gnome-source/README.md`. The acquisition
script records each Git commit in a `.revision` sidecar file under the local
`source/work/gnome` workspace.

The repository does not claim that a current upstream GNOME checkout is binary-
compatible with the older Linux 5.15.204 reference. Release integration will
select and pin a coherent set before a bootable MirvkBuntu image is declared.
