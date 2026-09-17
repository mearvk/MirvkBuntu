# Chromium White Edition — HTTP 3.0 Additional I/O Pane

## 1. Purpose

This document specifies the data contract for the secondary tasking bar beneath the URL/omnibox in the White Edition Chromium UI.

The bar is approximately **80% of the usable width of the URL input pane**, attaches directly beneath it, and presents compact, non-secret network/I/O information plus a modest request-variable review surface.

This specification is a UI and observation contract. It does not create a new wire protocol.

## 2. Visual and geometry contract

```text
┌───────────────────────────────────────────────────────────────┐
│ URL / omnibox                                                 │
├───────────────────────────────────────────────────────────────┤
│                                                               │
│        Secondary HTTP I/O / variable review pane              │
│        ≈ 80% of URL-pane usable width                         │
│                                                               │
├───────────────────────────────────────────────────────────────┤
│ web content                                                   │
└───────────────────────────────────────────────────────────────┘
```

The pane MUST attach directly to the URL/omnibox row and remain beneath it. It must not float over page content.

The width target is approximately 80% of the URL input pane's usable width. Responsive layout measurements MUST determine the actual width; 80% is not a fixed pixel requirement.

The variable-review field is a **modest text bar**, not a full developer console. It should be clean, crisp, lightly beveled, and visually integrated with the White Edition browser chrome.

### Bevel contract

- small corner radius;
- one restrained highlight edge;
- one restrained gray lower/outer shadow;
- no excessive glossy gradients;
- no animated bevel effects;
- sufficient contrast for text and focus states;
- consistent with the desktop's precision-lighting direction.

## 3. Data model

A logical observation record may contain:

```text
protocol
transport
connection_state
stream_state
origin
request_target
method
content_type
security_state
latency_ms
request_bytes
response_bytes
flow_state
cache_state
activity_state
error_code
error_text
normalization_schema
last_updated
```

All values are observational and should be treated as potentially transient.

## 4. Variable review bar

The second row has two logical regions:

```text
[HTTP state / compact controls] [ request-variable review text ]
```

The variable text field is read-only by default and shows a compact, escaped representation of the current request's GET or POST variables.

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

The field may expose a detail affordance to open the full normalized XML inspector. It must not expose secrets by default.

## 5. Four initial data cases

The UI MUST handle these initial cases:

| Case | Input | Review representation |
|---|---|---|
| JSON | `application/json` | Compact escaped JSON preview plus canonical XML representation. |
| XML | `application/xml` / `text/xml` | Validated XML preview plus canonical XML representation. |
| POST data | Form/body content | Normalized fields plus canonical XML representation. |
| GET data | URI query | Ordered query parameters plus canonical XML representation. |

The actual HTTP method and content type remain authoritative. These cases are normalization categories, not protocol replacements.

## 6. Canonical XML normalization

HTTP/1.0, HTTP/1.1, HTTP/2, and HTTP/3 observations are normalized into the project's bounded XML observation model before the browser UI consumes them.

The detailed contract is defined in:

`IO_XML_NORMALIZATION.md`

```text
HTTP/1.0 ─┐
HTTP/1.1 ─┤
HTTP/2   ─┼→ protocol observer → canonical XML → UI model → tasking bar
HTTP/3   ─┘
```

The XML is an internal data representation. It is not a new HTTP syntax and does not alter the bytes transmitted on the network.

## 7. Protocol field

`protocol` represents the protocol actually negotiated by Chromium.

Examples:

```text
h3
h2
http/1.1
unknown
```

For HTTP/3, `h3` corresponds to the ALPN identifier defined by RFC 9114. citeturn0search1

The UI MUST NOT display `h3` solely because the HTTP 3.0 feature is enabled.

## 8. Transport field

For HTTP/3, the transport is QUIC. HTTP/3 maps HTTP semantics onto QUIC, which supplies multiplexed streams and flow control. citeturn0search1

Example display:

```text
QUIC v1
```

Detailed diagnostics belong outside the primary strip.

## 9. Connection and stream state

Allowed connection presentation states:

```text
idle
connecting
negotiating
connected
reused
reconnecting
closed
failed
```

Suggested stream states:

```text
none
opening
requesting
receiving
streaming
complete
cancelled
failed
```

HTTP/3 maps request/response exchanges onto QUIC streams. citeturn0search1

## 10. HTTP method handling

The observer MUST preserve the actual HTTP method.

The initial UI explicitly supports:

```text
GET
POST
```

GET is treated as the retrieval method defined by HTTP semantics. POST carries request content for resource-specific processing. citeturn0search0turn0search3

A GET request body, if encountered, must not be silently interpreted as ordinary GET parameters. The URI query remains the normal GET-data source.

## 11. Security state

The pane may show a coarse state such as:

```text
secure
certificate-valid
certificate-warning
certificate-error
unknown
```

It MUST NOT expose:

- private keys;
- session tickets;
- TLS secrets;
- cookies;
- authorization credentials;
- bearer tokens;
- raw credential-bearing headers;
- decrypted secret material.

HTTP/3 relies on QUIC's TLS integration for confidentiality and integrity. citeturn0search1

## 12. Latency and transfer counters

`latency_ms` is an approximate presentation metric. It should not imply end-to-end precision that the underlying measurement cannot support.

Recommended displays:

```text
23 ms
↑ 4.2 KB
↓ 1.8 MB
```

or `—` when unavailable.

## 13. Flow, cache, and activity

Suggested flow states:

```text
normal
limited
backpressured
unknown
```

Suggested cache states:

```text
network
cache-hit
cache-revalidated
cache-miss
unknown
```

Suggested activity states:

```text
idle
loading
uploading
downloading
streaming
waiting
complete
failed
```

These are presentation states, not transport commands.

## 14. Error handling and fallback

Errors use a short label in the tasking bar and detailed diagnostics in a separate surface.

Example:

```text
HTTP/3 unavailable — using HTTP/2
```

The UI must report the actual negotiated protocol. HTTP/3 connectivity failure must not be represented as successful HTTP/3 merely because the feature is installed. HTTP/3 is standardized in RFC 9114 over QUIC. citeturn0search1

## 15. Responsive behavior

At full width, high-priority fields and the variable preview may be visible. At reduced widths, use this priority order:

1. method/protocol;
2. connection;
3. activity;
4. variable preview;
5. security;
6. stream;
7. latency;
8. transfer;
9. transport detail;
10. cache/flow detail.

The variable preview should shorten with an explicit ellipsis rather than causing horizontal overflow.

## 16. Accessibility

Every status has a text representation available to assistive technologies. Color alone MUST NOT communicate security, failure, or protocol state.

The pane must support keyboard focus and navigation without interfering with omnibox shortcuts.

## 17. Rendering

White Edition presentation:

- predominantly white surface;
- dark-gray text and controls;
- subtle gray elevation/shadow;
- restrained Ubuntu-red active/focus indication;
- slight curved bevel at the pane boundary;
- precision upper-left lighting consistent with the desktop LAF;
- no unnecessary animation.

The information pane should read as a **professional instrumentation strip with a compact variable review field**, not as a second developer toolbar.

## 18. Data ownership

The network/I/O observer owns the observation record. The UI owns presentation state only.

```text
Chromium network stack
        ↓
protocol observer
        ↓
normalization adapter
        ↓
HTTP 3.0 UI model
        ↓
secondary tasking bar
```

No UI event should directly mutate transport internals through this observation interface.

## 19. Versioning

The UI contract exposes a schema version independently of the negotiated HTTP protocol:

```text
ui_io_schema = 2
normalization_schema = 1
wire_protocol = negotiated
```

A future project-specific wire protocol must receive a separate protocol specification and compatibility matrix.

## 20. Reference standards

The interoperability baseline is standardized HTTP/3 from RFC 9114 over QUIC. HTTP semantics shared by HTTP/1.0, HTTP/1.1, HTTP/2, and HTTP/3 are defined by RFC 9110. citeturn0search1turn0search0
