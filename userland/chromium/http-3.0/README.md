# HTTP 3.0 — Chromium Additional I/O Pane

## Status

**Specification / build-integration stage.** This directory defines a project-specific HTTP I/O UI specification layered on standard HTTP/3. It does not redefine the IETF wire protocol.

The White Edition Chromium layout is:

```text
┌───────────────────────────────────────────────────────────────┐
│ tabs                                                          │
├───────────────────────────────────────────────────────────────┤
│ URL / omnibox                                      ≈ 100%     │
├───────────────────────────────────────────────────────────────┤
│ secondary tasking / HTTP I/O bar                    ≈ 80%     │
├───────────────────────────────────────────────────────────────┤
│                                                               │
│ web content                                                   │
└───────────────────────────────────────────────────────────────┘
```

The secondary tasking bar is approximately 80% of the usable URL/omnibox width and attaches directly beneath it. Responsive layout must calculate the actual width from current geometry rather than use a fixed device-pixel width.

## Build values

The checked-in build wrapper is `build-white-edition.sh`. It deliberately keeps generated Chromium output out of the source tree and uses GN/Ninja through Chromium's normal build system. Chromium's current Linux instructions use `gn gen` to generate build files and `autoninja -C <out> chrome` for the browser target. citeturn0search0turn0search2

The initial local-development values are:

```gn
is_debug = true
symbol_level = 1
blink_symbol_level = 0
v8_symbol_level = 0
is_official_build = false
is_chrome_branded = false
```

Project-specific switches remain commented until the corresponding source changes declare them:

```gn
# white_edition_ui = true
# white_edition_http_io = true
# white_edition_secondary_taskbar = true
# white_edition_http_io_width_ratio = 0.80
```

GN supports build-specific arguments in `out/<name>/args.gn` and provides `gn args <out> --list` to inspect available values. citeturn0search1turn0search4

See `BUILD_VALUES.md` for the complete value table and preflight rules.

## Additional I/O information pane

The secondary bar is a modest, clean, crisp review surface for HTTP variables and transport state. It uses a slight curved bevel, restrained gray depth, white base, dark-gray text, and restrained red active/error accents. It remains observational rather than becoming a developer console.

Recommended fields:

| Field | Purpose |
|---|---|
| Method | `GET` or `POST` request method. |
| Protocol | Actual negotiated `h3`, `h2`, or HTTP/1.x protocol. |
| Origin | Current origin/authority. |
| Variables | Safe, redacted representation of request variables. |
| Content | Normalized request/response content class. |
| Transport | QUIC/TCP transport state where available. |
| Security | TLS/security state without secrets. |
| Stream | Active request/response stream information. |
| Latency | Coarse request/connection timing. |
| Transfer | Request/response byte counters. |
| Activity | Current request/response/idle/reconnect/failure state. |

Sensitive cryptographic material, cookies, authorization credentials, session tickets, private keys, and equivalent secrets MUST NOT be displayed.

## Four initial input cases

The normalization layer handles four basic representations for both GET and POST observation:

1. **JSON** — structured JSON body/variable representation.
2. **XML** — XML body representation, parsed with external resource/entity resolution disabled.
3. **POST data** — form-style or equivalent POST variables.
4. **GET data** — URL query variables.

The normalized internal representation is XML for browser-side observation and UI consumption. This XML is an **internal data model**, not a replacement for HTTP wire semantics.

```text
HTTP/1.x / HTTP/2 / HTTP/3
           ↓
     method + headers
           ↓
      safe observer
           ↓
 JSON / XML / POST / GET
           ↓
   normalized XML model
           ↓
     I/O taskbar
```

The actual HTTP method and negotiated protocol remain authoritative. GET and POST must not be conflated merely because their variables are represented in one XML model.

## HTTP/3 interoperability baseline

The project uses the standardized HTTP/3 vocabulary: HTTP/3 runs over QUIC, uses `h3` ALPN negotiation, uses QUIC streams, uses QPACK for header compression, and has an HTTP control stream with SETTINGS. citeturn0search0

`HTTP 3.0` is therefore the project name for this UI/I/O feature layer unless a separate wire-protocol specification is created.

## I/O state model

```text
network event
     ↓
protocol observer
     ↓
request/response normalizer
     ↓
normalized XML observation
     ↓
Chromium UI model
     ↓
secondary tasking bar
```

Suggested states:

```text
IDLE
RESOLVING
CONNECTING
NEGOTIATING
CONNECTED
REQUESTING
RECEIVING
STREAMING
REUSED
RECONNECTING
FAILED
CLOSED
```

Rendering an indicator must not directly mutate transport state.

## Source/build boundary

```text
chromium-src/          pristine Chromium checkout
http-3.0/              project specifications/build contract
out/WhiteEdition/      generated GN/Ninja output
White Edition patches  deliberate source modifications
```

The preflight script checks for an expected Chromium layout and refuses an unexpected source-root `args.gn`, keeping generated configuration out-of-tree.

## Build sequence

```text
./build-white-edition.sh check
             ↓
./build-white-edition.sh gen
             ↓
         gn check
             ↓
./build-white-edition.sh build
             ↓
       chrome target
             ↓
./build-white-edition.sh test
             ↓
        unit_tests
```

The script does not claim a successful build until the commands actually complete. Chromium's documentation likewise distinguishes GN generation from the subsequent Ninja/autoninja compilation step. citeturn0search0

## Security

The UI information pane is observational. It must not weaken certificate validation, origin isolation, cookie protections, TLS handling, or QUIC security. The normalized XML layer must not execute scripts, resolve external entities/resources, or expose credentials.

## Fallback

The feature remains useful on HTTP/1.x and HTTP/2. The pane must report the actual negotiated protocol rather than claiming HTTP/3 merely because the feature is enabled. HTTP/3 connectivity depends on QUIC; fallback behavior must preserve ordinary HTTP compatibility.
