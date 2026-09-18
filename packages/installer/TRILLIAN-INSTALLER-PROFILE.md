# Trillian Installer Profile

Trillian/Dino is maintained as a local source-development component of the repository.

## Policy

- The installer profile does **not** silently download upstream source.
- `trillian/pull-dino.sh` is run explicitly when the source is wanted.
- The source is compiled from `trillian/dino` using the platform-specific build script.
- The profile does not perform a privileged system installation by itself.
- Build output remains under `trillian/build-*`.
- Upstream GPL-3.0 licensing and attribution remain applicable.

## Platforms

| Platform | Local build script |
|---|---|
| Linux | `trillian/build-linux.sh` |
| macOS | `trillian/build-macos.sh` |
| Windows | `trillian/build-windows.ps1` |

The master installer may invoke `installer/trillian-profile.sh` on Linux when the Trillian source has already been deliberately acquired. This keeps the component present in the Installer Profile without making source acquisition implicit.

## Boundary

This profile builds the open-source Dino foundation for project development. It does not claim to contain the historical proprietary Cerulean Studios Trillian source.
