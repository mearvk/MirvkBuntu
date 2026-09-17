# HTTP I/O → Canonical XML Normalization

## Purpose

The White Edition I/O pane provides a compact, read-only review surface for HTTP request variables. The browser may observe requests using HTTP/1.0, HTTP/1.1, HTTP/2, or HTTP/3, but the **internal review representation is normalized to a constrained XML document** before the UI consumes it.

This is a data-normalization layer, not a new HTTP wire protocol.

## Supported request cases

The initial implementation recognizes four presentation cases:

| Case | Typical source | XML representation |
|---|---|---|
| JSON | `application/json` | `<body type="json">…</body>` with JSON represented as escaped text or structured XML nodes. |
| XML | `application/xml`, `text/xml` | `<body type="xml">…</body>` after well-formedness validation. |
| POST data | Form or other request content | `<post-data>` containing method, content type, length and safely decoded fields where applicable. |
| GET data | URI query component | `<get-data>` containing the normalized query pairs. |

The four cases are presentation/normalization categories. They do not replace the actual HTTP method or media type.

## Canonical envelope

The normalized document MUST use a fixed root and explicit version:

```xml
<http-io schema="1" protocol="http/1.1" method="GET">
  <request>
    <origin>https://example.invalid</origin>
    <target>/search?q=example</target>
    <headers>
      <header name="accept">application/json</header>
    </headers>
    <get-data>
      <param name="q">example</param>
    </get-data>
    <body type="none" />
  </request>
</http-io>
```

For POST:

```xml
<http-io schema="1" protocol="http/2" method="POST">
  <request>
    <origin>https://example.invalid</origin>
    <target>/api/item</target>
    <headers>
      <header name="content-type">application/json</header>
    </headers>
    <post-data>
      <content-type>application/json</content-type>
      <body type="json">{&quot;name&quot;:&quot;example&quot;}</body>
    </post-data>
  </request>
</http-io>
```

## Safety boundary

The XML representation is **not executable markup**. It is a typed observation document.

The normalizer MUST:

1. preserve the actual negotiated HTTP protocol as metadata;
2. preserve the actual request method;
3. preserve media type and encoding metadata;
4. XML-escape text values;
5. reject malformed XML input rather than executing it;
6. treat JSON as data, not script;
7. avoid XML external entity resolution;
8. disable external DTD/entity fetching;
9. avoid network access while normalizing content;
10. redact or omit credentials and other secret material;
11. bound request/body/header sizes exposed to the UI;
12. make truncation explicit.

The normalizer must never transform an input into XML in a way that causes XML parsing to fetch external resources.

## GET handling

GET parameters are obtained from the URI query component. A GET request body must **not** be assumed to have normal semantics merely because bytes are present. HTTP semantics do not generally define request-content meaning for GET, and clients should not generate GET content unless the target origin has explicitly indicated support. citeturn0search3

Example:

```xml
<get-data>
  <param name="page">2</param>
  <param name="q">white edition</param>
</get-data>
```

Repeated query parameters are preserved in source order:

```xml
<get-data>
  <param name="tag">one</param>
  <param name="tag">two</param>
</get-data>
```

## POST handling

POST request content is normalized according to its declared media type. POST is defined by HTTP semantics as requesting resource-specific processing of the enclosed representation. citeturn0search3

Initial media types:

- `application/json`
- `application/xml`
- `text/xml`
- `application/x-www-form-urlencoded`
- `multipart/form-data` (metadata/field summary only in the initial UI)
- other media types: opaque, bounded preview only

The normalizer must not reinterpret arbitrary binary data as text.

## JSON handling

JSON may be represented as escaped source text for exact review or as a structurally validated XML projection. The projection must preserve JSON types:

```xml
<body type="json">
  <object>
    <string name="name">Example</string>
    <number name="count">4</number>
    <boolean name="enabled">true</boolean>
    <null name="value" />
  </object>
</body>
```

Arrays preserve order:

```xml
<array name="items">
  <string>one</string>
  <string>two</string>
</array>
```

No JSON-derived value is interpreted as executable code.

## XML handling

XML input is parsed only with external entity resolution and external resource access disabled. The canonical representation may retain validated XML as a bounded child representation or escaped text, depending on the renderer.

A malformed document becomes an explicit data error:

```xml
<body type="xml" status="malformed">
  <error>XML parse failure</error>
</body>
```

## Form POST handling

For `application/x-www-form-urlencoded`:

```xml
<post-data>
  <content-type>application/x-www-form-urlencoded</content-type>
  <field name="username">example</field>
  <field name="mode">white</field>
</post-data>
```

For multipart data, the initial UI should display field names, content types, sizes, and bounded text previews without exposing file contents or credentials by default.

## Header handling

Headers are metadata, not arbitrary XML element names. Header names therefore remain attribute values:

```xml
<header name="content-type">application/json</header>
```

The implementation MUST NOT create XML element names directly from untrusted header names or field names.

Sensitive fields should be represented as:

```xml
<header name="authorization" redacted="true">[redacted]</header>
```

At minimum, authorization credentials, cookies, proxy authorization, bearer tokens, and private cryptographic material are excluded or redacted.

## Protocol independence

The normalization layer accepts HTTP/1.x message observations, HTTP/2 stream observations, and HTTP/3 stream observations. The protocol-specific framing is handled before normalization:

```text
HTTP/1.0 ─┐
HTTP/1.1 ─┤
HTTP/2   ─┼→ protocol observer → normalized XML → I/O pane
HTTP/3   ─┘
```

HTTP/3 itself maps HTTP semantics onto QUIC and uses QUIC streams; the normalized XML must therefore describe the HTTP message while retaining HTTP/3/QUIC information as metadata rather than pretending that HTTP/3 is HTTP/1.1 syntax. citeturn0search1

## UI rendering contract

The normalized XML is an internal model. The visible pane should remain compact:

```text
┌─────────────────────────────────────────────────────────────────────┐
│ GET   JSON   200   ↑ 1.2 KB   ↓ 18.4 KB   h3   23 ms              │
└─────────────────────────────────────────────────────────────────────┘
```

Clicking/focusing the pane may open the detailed variable inspector, but the primary strip should not become a large developer console.

## Truncation

All UI previews are bounded. A truncation marker must identify omitted data:

```xml
<body truncated="true" original-bytes="183920" preview-bytes="8192">…</body>
```

The raw request remains owned by Chromium's network stack; the XML representation is only a bounded observation copy.

## Versioning

```text
normalization_schema = 1
```

Changes to element names, typing, redaction rules, or semantics require a schema revision. The negotiated HTTP protocol is independent metadata and is never inferred from the normalization schema version.
