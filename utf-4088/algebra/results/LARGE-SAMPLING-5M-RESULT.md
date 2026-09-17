# UTF-4088 Large Sampling — Executed Result

## Run

- samples: **5,000,000**
- seed: `0x40880812`
- glyph geometry: 8×12 = 96 binary pixels
- sampler: `large_summary` equivalent to the repository large-map topology logic
- output: `large_sampling_5m_summary.csv`

## Observed result

| Metric | Result |
|---|---:|
| Samples | 5,000,000 |
| 4-connected glyphs | 1,316 |
| Connected rate | 0.02632% |
| 8–40 black-pixel glyphs | 313,927 |
| 8–40 density rate | 6.27854% |
| Full interpretability candidate filter | **0** |
| Candidate rate | **0** |

The candidate filter was:

`connected AND 8<=black_pixels<=40 AND 8<=transitions<=90 AND edge_count>0`

## Important finding

The geometric filter as currently defined is **too restrictive in combination** for random uniform 96-bit sampling: five million samples produced no glyph satisfying all four conditions.

This does not contradict the raw 96-bit capacity result. It means that the current candidate definition is a poor random-space filter for discovering the desired population.

The connected rate is consistent with the earlier million-sample experiment:

- earlier: 276 / 1,000,000 = 0.02760%
- this run: 1,316 / 5,000,000 = 0.02632%

The agreement is useful evidence that the connectivity statistic is stable under the deterministic generator.

## Consequence for the 4-billion target

A zero-observation candidate rate cannot be extrapolated into a population estimate. Therefore this run does **not** establish that the current four-condition candidate filter yields four billion symbols.

Raw capacity remains overwhelmingly sufficient:

`2^96 / 4,000,000,000 ≈ 1.98e19`

The next experiment should therefore change the sampling strategy rather than simply increase uniform random sample count. In particular, sample **conditioned/constructed connected glyphs** first, then measure the transition and density distributions. That directly explores the graph-valid region instead of spending almost all samples on disconnected bitmaps.

## Status

- Raw capacity: **PASS**
- Connectivity feasibility: **SUPPORTED**
- Current random-space combined candidate filter: **NOT DEMONSTRATED**
- Four-billion interpreted namespace: **NOT PROVEN**

The result is intentionally preserved without converting zero observations into an unsupported extrapolation.
