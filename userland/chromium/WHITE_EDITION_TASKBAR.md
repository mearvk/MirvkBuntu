# Chromium — Ubuntu White Edition Secondary Tasking Bar

## Purpose

Ubuntu White Edition adds a **secondary tasking/I/O bar immediately beneath Chromium's URL/omnibox area**. This is a browser-chrome feature, not a web-page toolbar and not an extension injected into page content.

The bar is approximately **80% of the usable width of the URL input pane** and attaches directly beneath it. It provides a compact task surface plus a modest text field for reviewing the variables associated with the current HTTP request.

## Placement

```text
┌─────────────────────────────────────────────────────────────┐
│ Tabs                                                        │
├─────────────────────────────────────────────────────────────┤
│ Navigation / URL (omnibox)                                  │
├─────────────────────────────────────────────────────────────┤
│        WHITE EDITION SECONDARY TASKING / HTTP I/O BAR       │
│ [Back] [Forward] [Reload] [Tasks] [↓] [ HTTP variables... ]│
├─────────────────────────────────────────────────────────────┤
│ Web content                                                 │
└─────────────────────────────────────────────────────────────┘
```

The tasking bar remains **directly below the URL input/omnibox row**. It does not replace the omnibox, alter URL entry semantics, or overlay page content.

## Geometry

The URL/omnibox row provides the reference width. The secondary bar targets approximately 80% of that usable width.

```text
URL / omnibox                 100%
Secondary tasking / I/O bar    ≈80%
```

The 80% value is responsive rather than a fixed pixel width. Chromium layout code must calculate the current usable geometry.

## Variable review field

The right-hand or trailing portion of the secondary bar contains a modest read-only text field for reviewing HTTP variables.

Examples:

```text
GET  q=ubuntu&page=2
```

```text
POST  {"name":"Example","enabled":true}
```

```text
POST  username=example&mode=white
```

The field is intentionally compact. Selecting/focusing it may open the detailed I/O inspector, whose canonical internal representation is XML. The primary bar must not become a full developer console.

## Supported request cases

The initial implementation handles both **GET and POST**, with four normalization cases:

| Case | Source | Review behavior |
|---|---|---|
| JSON | `application/json` | Compact JSON preview and canonical XML normalization. |
| XML | `application/xml`, `text/xml` | Validated XML preview and canonical XML normalization. |
| POST data | Form/body content | Normalized fields and canonical XML normalization. |
| GET data | URI query | Ordered query parameters and canonical XML normalization. |

HTTP/1.0, HTTP/1.1, HTTP/2, and HTTP/3 observations may feed the same normalization layer. The actual negotiated protocol and actual method remain authoritative.

Detailed normalization rules are in:

`http-3.0/IO_XML_NORMALIZATION.md`

## Visual contract

The bar follows the White Edition desktop language:

- predominantly white surface;
- dark-gray text and controls;
- neutral gray separators/shadows;
- restrained Ubuntu-red active/focus indication;
- slight curved bevel directly at the bar boundary;
- precise but modest upper-left lighting;
- crisp typography and compact spacing;
- no excessive gradients that reduce legibility.

The bevel should read as a precision-machined UI edge, not as a glossy decorative effect.

## Interaction contract

The tasking bar is a native Chromium UI surface. Controls use Chromium's normal command, focus, accessibility, keyboard-navigation, tooltip, and accelerator systems wherever applicable.

| Control | Function | Initial state |
|---|---|---|
| Back | Navigate backward | Enabled when history permits. |
| Forward | Navigate forward | Enabled when history permits. |
| Reload | Reload current page | Enabled. |
| Tasks | White Edition task surface | Reserved for project integration. |
| HTTP I/O | Show/inspect request variables | Enabled for supported GET/POST observations. |
| Downloads | Open Chromium downloads | Enabled when supported by the build. |
| Overflow | Additional actions | Reserved for future commands. |

The feature does not introduce a parallel browser command model. Existing Chromium commands remain authoritative.

## I/O observation boundary

The I/O information is read-only observation data:

```text
Chromium network stack
        ↓
protocol observer
        ↓
GET/POST + JSON/XML/form/query normalization
        ↓
canonical XML observation
        ↓
compact taskbar preview
```

The UI does not directly modify network transport state merely by displaying an observation.

Sensitive material such as cookies, authorization credentials, bearer tokens, private keys, session tickets, and raw secret-bearing headers must be redacted or omitted.

## Source integration boundary

Chromium is fetched into:

```text
userland/chromium/chromium-src/
```

The repository currently stores the Chromium acquisition/build wrapper rather than a permanently vendored Chromium source tree. The customization is therefore maintained outside the upstream source tree and applied during the local build process.

The implementation belongs in Chromium's native browser-view/toolbar hierarchy. A customization script must inspect the fetched Chromium revision before modifying it and refuse to patch an unrecognized source layout.

## Safety requirements

1. Record the Chromium Git revision before customization.
2. Require expected source-layout markers before applying changes.
3. Keep project customization outside the pristine upstream source where practical.
4. Make customization idempotent; a second run must not duplicate the bar.
5. Fail closed when expected Chromium UI classes/files are absent.
6. Preserve a clean path to rebuild Chromium without the White Edition customization.
7. Report customization status separately from ordinary Chromium build success.
8. Never treat the HTTP I/O pane as a mechanism for bypassing Chromium's origin, certificate, cookie, or content-security protections.
9. Do not expose secrets through the variable preview.

## Build relationship

```text
fetch Chromium
      ↓
record revision
      ↓
verify expected UI source layout
      ↓
apply White Edition taskbar customization
      ↓
apply HTTP I/O observation UI
      ↓
GN/Ninja build
      ↓
Chromium executable
      ↓
ISO/rootfs packaging
```

This document defines the architectural contract. It does not claim that the native Chromium modification has already been compiled merely because the specification exists.

## Planned customization boundary

```text
userland/chromium/
├── WHITE_EDITION_TASKBAR.md
├── white-edition/
│   ├── README.md
│   └── apply-taskbar.sh
├── http-3.0/
│   ├── README.md
│   ├── UI_IO_SPEC.md
│   └── IO_XML_NORMALIZATION.md
└── chromium-src/
    └── <fetched Chromium source>
```

The `white-edition/` and `http-3.0/` layers remain separate from Chromium upstream so the upstream source can be refreshed without losing the project's desktop customization.
