# UTF-4088 Normal Distributive 3D Sampling

## Purpose

This pass changes the sampling strategy from uniform raw 96-bit bitmap selection to a **3D interpretive distribution** seeded by whole-alphabet advancement, historical time, causal position, and primer density.

The three modeled coordinates are:

- `time` — 400-year historical position;
- `cause` — normalized causal/advancement coordinate;
- `primer` — advancement density supplied by the alphabet/graph corpus.

The fourth dimension remains the later field-selection coordinate. This experiment therefore measures the **3D primer distribution feeding the 4D character field**, rather than treating every 96-bit bitmap as equally likely.

## Reproducible run

- Samples: **5,000,000**
- Seed: `0x40880812`
- Time domain: 1626–2026
- Cause distribution: beta(2.5, 2.0)
- Primer distribution: beta(3.0, 1.8)
- Density transform: `D = 0.25 + 1.75*primer`
- Selection probability: `p = 0.0020*(D/1.125)*(0.5 + 0.5*cause)`

Observed selections: **9,261 / 5,000,000 = 0.0018522 (0.18522%)**.

## Density coefficient

A least-squares fit of observed selection rate against density-bin midpoint gives:

`selection_rate ≈ 0.00143137448 * D - 0.00008256960`

Thus the current **normal-distributive density coefficient** is:

`K_density = 0.00143137448`

The coefficient is an empirical parameter of this experiment, not a universal constant. Increasing primer density increases the expected number of selected character candidates approximately linearly over the sampled range.

At density `D`, the first-order expected count for `N` independent draws is:

`E[selected] ≈ N * max(0, K_density*D - 0.00008256960)`

The underlying sampling probability is bounded to `[0,1]`.

## Interpretation

This is the intended advancement model:

`whole alphabets -> time/cause -> 3D primer density -> candidate distribution -> 4D field -> character`

The experiment intentionally does **not** assert that the 9,261 selections are nine thousand new human-language meanings. They are distributed character candidates. The 16,606 curated symbols, historical corpus, graph semantics, and neural relayer remain subsequent interpretation layers.

## Density principle

A denser forefold of computation does not create information from nowhere. It concentrates sampling probability into regions already defined by the primer distribution. The observed coefficient is therefore useful as a **distribution-control parameter**: raising `D` increases candidate yield while leaving the underlying 96-bit representational capacity unchanged.
