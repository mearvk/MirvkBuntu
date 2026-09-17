# Vala Source

This directory tracks the upstream Vala compiler and language tooling used by the Ubuntu Determinant GNOME build.

Vala is a programming language and compiler that generates C source for use with GLib and GNOME libraries. It is a build/development dependency, not the GNOME desktop shell itself.

## Layout

```text
vala/
├── README.md
├── pull-source.sh
└── upstream/
    └── vala source
```

The pull script acquires pristine upstream source without requiring interactive Git credentials. Future Ubuntu Determinant changes should be maintained as separate patches or offsets rather than mixed into `upstream/`.

## Source policy

- Pull from the official Vala upstream repository.
- Use HTTPS and disable configured credential helpers by default for public source.
- Support an explicit branch, tag, or commit through `VALA_SOURCE_REF`.
- Support shallow clones through `VALA_CLONE_DEPTH`.
- Record the exact checked-out commit in `.source-commit` and `SOURCE-INFO.txt`.
- Preserve upstream licensing and attribution.
