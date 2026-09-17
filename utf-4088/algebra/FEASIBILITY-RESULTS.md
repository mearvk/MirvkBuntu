# UTF-4088 4-Billion-Symbol Feasibility Test

## Scope

This test evaluates whether a binary 8x12 character cell (96 independent black/white pixels) has sufficient combinatorial capacity for a target set of 4,000,000,000 distinct symbols.

It is a **capacity and graph-feasibility test**, not proof that four billion symbols have already been assigned human meanings.

## Raw capacity

A 96-bit cell has:

`2^96 = 79,228,162,514,264,337,593,543,950,336`

possible binary patterns.

Relative to the target:

`2^96 / 4,000,000,000 ≈ 1.9807e19`

Thus the raw visual space exceeds the target by approximately 19.8 quintillion times.

## Reproducible sample

The repository sampler uses a fixed deterministic seed and samples 1,000,000 96-bit states. Each state is converted to an 8x12 glyph and evaluated by the existing topology analyzer.

Observed sample result:

| Measure | Result |
|---|---:|
| Samples | 1,000,000 |
| Non-empty | 1,000,000 |
| 4-connected | 276 |
| 4-connected + 4..48 black pixels | 1 |
| Sampled states | 1,000,000 |

The observed 4-connected rate is:

`276 / 1,000,000 = 0.000276 = 0.0276%`

If this rate were representative of the entire space, the extrapolated connected population would be approximately:

`2^96 * 0.000276 ≈ 2.19e25`

which remains enormously larger than four billion.

The stricter `connected + 4..48 black pixels` observation occurred only once in the million-sample run and therefore is **not statistically sufficient for a reliable population-rate estimate**. It should not be used as evidence for a precise yield.

## Interpretation

The result establishes that 4 billion distinct 8x12 binary symbols are feasible at the level of combinatorial capacity and that a substantial connected subset can exist under the current 4-neighbor graph rule.

It does **not** establish that four billion glyphs are all:

- visually distinct to humans,
- linguistically meaningful,
- historically grounded,
- semantically useful,
- culturally interpretable, or
- appropriate as additions to Unicode.

Those properties require separate validation layers.

## Design consequence

The project should therefore reserve a 4-billion-member **interpreted character namespace** inside the much larger 96-bit visual space. Selection can be deterministic and versioned while leaving substantial space for graph variants, historical variants, collision avoidance, and future extensions.

The existing 16,606-symbol front end remains the explicitly curated layer. The four-billion target is the experimental remainder capacity.

## Reproduction

Build the sampler together with `glyph8x12.cpp`, then run:

```text
space_sampler 1000000 utf4088-space-sample.csv
```

The CSV records per-sample black-pixel count, connected components, connectivity, edge count, transition count, and deterministic signature.

## Feasibility conclusion

**PASS — capacity feasible.**

**CONDITIONAL PASS — graph-feasible subset is sufficiently large under the observed 4-connected sample rate.**

**NOT YET PROVEN — four billion semantically interpreted characters.**
