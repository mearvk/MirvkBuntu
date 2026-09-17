# Historical Source Specimens

This catalog records dated digitized sources that can serve as provenance anchors for the UTF-4088 glyph corpus. The project must distinguish the historical source image from any later 8x12 rasterization.

## Korean — 1888

**전보장정 (Telegraph Regulations), 1888**

- Date: 1888 (Gojong 25)
- Holder/administrator: National Library of Korea / Korean Heritage Digital Service records
- Significance: contains the Korean letter telegraph system known as `국문자모 호마타법`, described by the Korean Heritage Digital Service as a binary dot/dash system devised by Kim Hak-u for consonants and vowels.
- Digital source: Korean Heritage Digital Service record `전보장정(2012-1)`.
- Source: https://digital.khs.go.kr/heri/heriDetail.do?ctptNo=4411105290100&ctptUid=13983497600799027442

**한성주보 (Hansŏng Chubo), 1886–1888**

- Publication period: 1886–1888
- Significance: contemporary Korean-language newspaper material from the requested historical period. Scholarship identifies it as an early modern publication using Korean-script articles.
- Digital discovery source: National Library of Korea Korean Newspaper Archive; Seoul Museum historical description.
- Sources:
  - https://www.nl.go.kr/EN/contents/EN35200000000.do
  - https://museum.seoul.go.kr/www/board/NR_boardView.do?bbsCd=1182&seq=20221212103825622&sso=ok

The corpus should use the actual page images from these collections when producing historical 8x12 samples. A generated raster must retain the source identifier and page number.

## German — 1872

**Konrad Duden, Die deutsche Rechtschreibung, 1872**

- Date: 1872
- Publisher: B. G. Teubner
- Significance: Duden's first orthographic work, commonly called the Schleizer Duden, containing rules and a word list.
- Digitized copy: Google Books, scanned from University Library Basel.
- Source: https://books.google.com/books/about/Die_deutsche_Rechtschreibung.html?id=DG9cE6TDABMC

**Regeln und Wörterverzeichnis für die deutsche Orthographie, Berlin 1872**

- Date: 1872
- Edition: 3rd edition
- Digitized by: Bayerische Staatsbibliothek
- Identifier: urn:nbn:de:bvb:12-bsb11009980-2
- Digital source: Deutsche Digitale Bibliothek.
- Source: https://www.deutsche-digitale-bibliothek.de/item/ZMDDSJEMUGHJC6PRTOWYD7ZURMBWCWTW

## Rasterization rule

No historical glyph is to be treated as an 8x12 character until the original page/image has been captured, cropped, deskewed, and downsampled using a recorded procedure. Each derived glyph should retain:

`source_id + edition/date + page + crop + rasterization_version + 8x12_bitmap`

This prevents the experimental UTF-4088 representation from being confused with the historical source itself.
