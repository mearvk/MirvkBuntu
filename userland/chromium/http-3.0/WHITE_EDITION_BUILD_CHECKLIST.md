# White Edition Chromium Build Checklist

Use this checklist before calling a Chromium White Edition build complete.

## Source

- [ ] Chromium checkout exists at the configured `CHROMIUM_SRC`.
- [ ] `DEPS` exists.
- [ ] Root `BUILD.gn` exists.
- [ ] `chrome/`, `content/`, and `components/` exist.
- [ ] No generated `out/` tree has been placed inside the source checkout.
- [ ] Source revision is recorded.

## Toolchain

- [ ] `gn` is available.
- [ ] `autoninja` is available.
- [ ] Chromium Linux build dependencies are installed.
- [ ] Chromium hooks/dependencies have been synchronized where required.

Chromium's Linux documentation calls for installing build dependencies and running the Chromium hooks after checkout before building. citeturn0search2

## GN

- [ ] Output directory is one of the White Edition output directories.
- [ ] `gn gen` succeeds.
- [ ] `gn args <out> --list` has been reviewed.
- [ ] No undefined project-specific GN argument has been passed.
- [ ] White Edition arguments are enabled only after the corresponding source patch exists.

## Build

- [ ] `autoninja -C <out> chrome` succeeds.
- [ ] `autoninja -C <out> unit_tests` succeeds.
- [ ] Expected Chromium binary exists.

Chromium documents `autoninja -C out/Default chrome` and the corresponding `unit_tests` target as the normal local build/test path. citeturn0search0

## White Edition UI

- [ ] Secondary tasking bar is directly beneath the URL/omnibox.
- [ ] Nominal width is 80% of the URL pane's usable width.
- [ ] Bevel is modest and visually crisp.
- [ ] White base, gray depth, and restrained red focus treatment are applied.
- [ ] UI remains usable at narrow and wide window sizes.
- [ ] Keyboard navigation reaches the I/O review field.
- [ ] Accessibility labels describe the I/O review field.

## HTTP I/O

- [ ] GET is recognized.
- [ ] POST is recognized.
- [ ] JSON input is normalized.
- [ ] XML input is normalized.
- [ ] Form POST data is normalized.
- [ ] GET query data is normalized.
- [ ] HTTP/1.0, HTTP/1.1, HTTP/2, and HTTP/3 observations can enter the common review model.
- [ ] Normalization does not change the wire protocol.
- [ ] XML external entities/resources are disabled.
- [ ] Credentials, cookies, authorization values, and sensitive headers are redacted.
- [ ] Size and nesting limits are enforced.

## Final status

Do not mark `BUILT`, `TESTED`, `PACKAGED`, or `BOOT VERIFIED` until the corresponding command or runtime check has actually completed and produced evidence.
