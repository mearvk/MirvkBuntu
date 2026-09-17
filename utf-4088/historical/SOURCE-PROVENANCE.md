# Historical Source Provenance

This directory distinguishes historical evidence from generated candidate glyphs.

## Korean / Hangul

The National Hangeul Museum's public guide documents the original Hunminjeongeum system and its graphic principles: 17 consonants and 11 vowels in the original 28-letter system, with basic consonants modeled on speech organs and basic vowels on heaven, earth, and human. The museum also provides a public guide and explanatory material for the system.

For the UTF-4088 corpus, historical Korean glyphs must be captured from a dated primary or archival facsimile before being labeled as an 1888 specimen. The current candidate generator is therefore marked `procedural`, not `historical`.

## German / Germanic

Konrad Duden's official history identifies 1872 as the publication year of *Die deutsche Rechtschreibung*, and 1880 as the publication of the *Urduden*. These are useful dated anchors for the German orthographic layer. They establish historical orthographic provenance, but they do not by themselves provide a complete historical glyph image set.

## American English

The project treats the American-English layer separately from historical German and Korean sources. A dated source edition must be recorded before a glyph is called historically authentic.

## Corpus rule

Each glyph record must carry:

- source identifier
- source year
- source page/image reference where available
- transcription
- normalization status
- whether the 8x12 bitmap is traced, rasterized, or procedurally generated
- copyright/licensing status

No generated bitmap is to be described as a historical original merely because its seed string corresponds to a historical letter.
