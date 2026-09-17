# UTF-4088 Interpretability Sampling

This experiment moves beyond raw 96-bit capacity and samples for a conservative candidate glyph population.

## Candidate filter

A sampled glyph is a candidate when:

- it is 4-neighbor connected;
- it contains 8–40 black pixels;
- it has 8–90 horizontal/vertical transitions;
- it has at least one graph edge.

These are **engineering filters**, not linguistic truth criteria. They favor compact, connected, graph-expressive forms while rejecting empty, extremely dense, or trivially unstructured bitmaps.

## Sample

Default run:

`interpretability_sampler 5000000 utf4088-interpretability-sample.csv`

The seed is deterministic (`0x40880812`) so subsequent runs are reproducible.

The CSV contains per-sample topology measurements and candidate classification. The executable prints the observed rates.

## Interpretation

The candidate rate is the quantity to compare against the 4-billion target after extrapolation. Because the sample is intentionally random over the 96-bit space, it does not establish semantic meaning. A later corpus stage must compare candidates against the 16,606 curated symbols, historical forms, graph structures, and language priors.

The resulting model is therefore:

`96-bit capacity -> graph-valid candidates -> visually constrained candidates -> historical/language matching -> interpreted namespace`

A successful candidate-rate test establishes feasibility of the intermediate population; it does not claim that arbitrary generated glyphs are human language.
