# Determinant Extended Parent-Path Policy

Ubuntu/Linux keeps the normal meaning of `..` unchanged: one parent directory.

Ubuntu Determinant adds two optional shorthand components for paths handled by
Determinant-aware UI/tools:

- `...`  -> two parent levels (`../..`)
- `....` -> three parent levels (`../../..`)

These are **not changes to Linux kernel pathname semantics**. A literal `...`
or `....` remains a valid ordinary filename to the kernel. The Determinant
path-policy layer recognizes these tokens before passing a path to normal
filesystem APIs.

## Examples

From `/a/b/c/d`:

```text
..        => /a/b/c
...       => /a/b
....      => /a
x/...     => /a/b/c/x/../..
```

The implementation treats each token as a whole path component. It does not
rewrite `foo...bar`, `...txt`, or other ordinary filenames containing dots.

## Integration boundary

```text
GNOME / Nautilus / Determinant UI
             |
      path-policy expansion
             |
       GIO / libc / kernel
```

The kernel and the conventional `..` behavior remain untouched. This keeps
normal Linux compatibility while allowing the Ubuntu White Edition desktop to
adopt the extended navigation vocabulary in Determinant-aware components.

The helper is intentionally small so it can later be called from a GNOME
extension, Nautilus integration, or other desktop navigation component without
forking the Linux VFS.
