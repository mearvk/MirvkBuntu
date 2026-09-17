# Chromium White Edition Build Values

This file defines the initial local-development build values for the Chromium integration. They are deliberately conservative: the build is a real Chromium build configuration, while project-specific White Edition GN switches remain commented until the corresponding patched Chromium source declares them.

## Source and output

| Value | Default | Purpose |
|---|---|---|
| `CHROMIUM_SRC` | `userland/chromium/chromium-src` | Chromium checkout. |
| `CHROMIUM_OUT` | `<CHROMIUM_SRC>/out/WhiteEdition` | Generated GN/Ninja output. |
| `TARGET_CPU` | host default | Optional explicit target CPU. |
| `BUILD_JOBS` | tool default | Optional `autoninja -j` limit. |

## GN values

```gn
is_debug = true
symbol_level = 1
blink_symbol_level = 0
v8_symbol_level = 0
is_official_build = false
is_chrome_branded = false
```

These values provide a practical development build with reduced debug-symbol volume while retaining assertions. Chromium's current Linux build documentation uses GN to generate Ninja files and recommends `autoninja -C out/Default chrome` for the browser target. citeturn0search0turn0search1

## White Edition values reserved for source integration

These remain commented in `args.gn` until the patched Chromium source declares them:

```gn
# white_edition_ui = true
# white_edition_http_io = true
# white_edition_secondary_taskbar = true
# white_edition_http_io_width_ratio = 0.80
```

This avoids passing undefined GN arguments to an unmodified Chromium checkout. GN records build arguments in the output directory's `args.gn`; available arguments should be checked with `gn args <out> --list`. citeturn0search1turn0search4

## Build sequence

```text
check
  ↓
gn gen
  ↓
gn check
  ↓
autoninja chrome
  ↓
autoninja unit_tests
```

The preflight checks for `DEPS`, `BUILD.gn`, `chrome/`, `content/`, and `components/`, and rejects an unexpected source-root `args.gn` so generated configuration remains out-of-tree.

## Commands

```bash
./build-white-edition.sh check
./build-white-edition.sh gen
./build-white-edition.sh build
./build-white-edition.sh test
```

The script is intentionally a source/build wrapper, not a claim that the Chromium checkout is already present or that the build has already succeeded.
