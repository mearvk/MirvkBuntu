# Chromium White Edition Build Matrix

This document turns the initial Chromium White Edition build values into explicit development, test, and packaging profiles. It follows Chromium's current GN + Siso/Ninja workflow: generate an output directory with `gn gen`, then build the `chrome` target with `autoninja`. citeturn0search0

## Profiles

| Profile | Debug | Symbols | Official | Branded | Intended use |
|---|---:|---:|---:|---:|---|
| `white-dev` | true | 1 / Blink 0 / V8 0 | false | false | Primary local UI/I/O development. |
| `white-test` | true | 1 / Blink 0 / V8 0 | false | false | UI and HTTP normalization tests. |
| `white-release-prep` | false | 1 / Blink 0 / V8 0 | false | false | Release-oriented integration check. |
| `white-release` | false | 0 / Blink 0 / V8 0 | true | false | Final release candidate; only after source integration. |

`white-dev` is the default. The reduced symbol settings are consistent with Chromium's documented guidance for faster builds while retaining useful stack information. citeturn0search0

## Common GN values

```gn
is_chrome_branded = false
```

Development and test builds should not enable project-specific GN arguments until the patched Chromium source declares them. Once integrated, the White Edition source may declare:

```gn
white_edition_ui = true
white_edition_http_io = true
white_edition_secondary_taskbar = true
white_edition_http_io_width_ratio = 0.80
```

These are **reserved project arguments**, not claims that stock Chromium currently accepts them.

## Output directories

```text
out/WhiteEditionDev
out/WhiteEditionTest
out/WhiteEditionReleasePrep
out/WhiteEditionRelease
```

Each profile is isolated so generated `args.gn`, Ninja/Siso metadata, objects, and binaries never contaminate the Chromium source tree.

## Required checks

Before generation:

```text
DEPS
BUILD.gn
chrome/
content/
components/
```

After generation:

```bash
gn args out/WhiteEditionDev --list
gn ls out/WhiteEditionDev
```

Chromium documents `gn ls` for discovering available build targets. citeturn0search0

## Build sequence

```bash
./build-white-edition.sh check
./build-white-edition.sh gen
./build-white-edition.sh build
./build-white-edition.sh test
```

The underlying browser build is:

```bash
autoninja -C out/WhiteEditionDev chrome
```

Chromium's current Linux documentation explicitly recommends `autoninja` for the browser target. citeturn0search0

## Verification targets

At minimum, the local verification stage should cover:

1. GN generation succeeds.
2. The `chrome` target compiles.
3. `unit_tests` compiles.
4. HTTP GET normalization tests pass.
5. HTTP POST normalization tests pass.
6. JSON input normalization tests pass.
7. XML input normalization tests pass.
8. Form POST data normalization tests pass.
9. GET query-data normalization tests pass.
10. The secondary taskbar is exactly attached beneath the omnibox in the patched UI implementation.
11. The taskbar's nominal width is 80% of the URL pane's usable width.
12. Sensitive values are redacted in the review pane.

Chromium's documented test flow builds `unit_tests` with `autoninja -C out/Default unit_tests`; the White Edition wrapper should use the corresponding selected output directory. citeturn0search0

## Source integrity

The build wrapper must fail rather than silently adapting to an unexpected Chromium source layout. The following are build errors, not warnings:

- missing `DEPS`;
- missing root `BUILD.gn`;
- missing `chrome/`, `content/`, or `components/`;
- generated output located inside the source tree;
- missing `gn` or `autoninja`;
- undefined White Edition GN arguments passed to an unpatched source tree;
- attempting to report a build as successful without the expected artifact.

## Status terminology

`CONFIGURED` means GN arguments have been generated. `BUILT` means the requested target completed. `TESTED` means the declared tests completed. `PACKAGED` means the resulting artifact has been staged. None of these statuses should be inferred merely from the existence of a script.
