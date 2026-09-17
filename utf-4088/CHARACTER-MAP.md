# UTF-4088 Character Map

## Status
Experimental character-system specification. UTF-4088 is not a Unicode standard and its generated symbols are not asserted to be universally valid characters until independently reviewed and registered.

## 16,606-symbol front-end set

The front end reserves exactly **16,606 published symbol records**. Each record now carries both a machine-readable pixel representation and an explicit human-readable description.

Required fields:

1. `integer_id` — stable published identifier.
2. `stage` — `start`, `intermediate`, or `final`.
3. `language_set` — one or more of `american-english`, `korean`, `germanic`.
4. `shape_id` — canonical shape identifier.
5. `meaning_id` — semantic/concept reference.
6. `codepoint` — UTF-4088 experimental code-space value.
7. `render_policy` — deterministic rendering instructions.
8. `pixel_width` — glyph width in pixels; the baseline experimental form is **8**.
9. `pixel_height` — glyph height in pixels; the baseline experimental form is **12**.
10. `pixel_rows` — exactly `pixel_height` rows, each containing exactly `pixel_width` binary pixels (`0` = white/background, `1` = black/stroke).
11. `pixel_bitmap` — canonical row-major binary representation of the complete glyph.
12. `description` — concise human-readable description of the symbol's visible form and, where established, its semantic role.
13. `description_status` — one of `structural`, `annotated`, or `validated`; a structural description must not be mistaken for an established linguistic meaning.
14. `source_reference` — provenance for the shape/meaning annotation when applicable.

### Pixel-level canonical form

The baseline glyph is an **8×12** binary cell:

```text
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
xxxxxxxx
```

Each `x` is one pixel. The canonical serialized form is 12 rows concatenated from top to bottom, left to right. This makes the visual character independently reproducible without relying on a font renderer.

A record may also expose an ASCII preview for review:

```text
..##....
.####...
...##...
...##...
..####..
....##..
....##..
...###..
...#....
...#....
...#....
..###...
```

The preview is illustrative; the authoritative representation is `pixel_rows` / `pixel_bitmap`.

### Description rule

The description has two distinct layers:

- **Structural description:** what the pixels and graph actually show, such as `connected diagonal stroke with two branches`.
- **Semantic description:** what a reviewed registry or corpus explicitly assigns to the symbol, such as a language-specific character or concept.

A shape must not receive a semantic meaning merely because a neural process or geometric classifier predicts one. This distinction preserves the integrity of the 16,606-symbol front end.

The three-language set is treated as an integral tuple `(English/American, Korean, Germanic)`. Existing standardized characters remain references to their established encodings; UTF-4088 does not redefine Unicode code points.

## Gradation matrix

Every symbol is assigned a three-stage gradation:

`start → intermediate → final`

The matrix is ordered first by language tuple, then shape, then semantic relationship, then rendering precision. A final symbol is immutable once published. Intermediate symbols are permitted as derivational states and start symbols represent the entry state of a representation sequence.

## Large experimental remainder

The remaining experimental code space is generated rather than exhaustively materialized. A 4D address is represented conceptually as:

`Q = (x, y, p, v)`

where `x,y` identify location, `p` is the normalized pressure/field-derived parameter, and `v` is normalized voltage state. The renderer maps `Q` through a deterministic quantizer and shape generator.

The phrase "four billion characters" refers to a conceptual code-space capacity, not a claim that four billion human-language meanings have already been identified.

## Internationalization

The front-end language tuple is deliberately plural. No language receives an intrinsic semantic priority. Meaning is represented by the registry and graph relationships, while shape is represented independently so that the same concept may have multiple linguistic realizations.

## Publication requirement

The 16,606-symbol registry is not considered complete merely because 16,606 integer identifiers exist. A release candidate must provide a complete pixel representation and description for every identifier, with provenance and description status. Missing pixel data or unsupported semantic descriptions must remain explicitly marked rather than inferred or fabricated.
