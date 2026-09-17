# Strong Natural Die Unit

For the experimental UTF-4088 field model, the **natural die unit (NDU)** is a deterministic, dimensionless quantization interval used to turn a measured field state into a stable symbolic state.

## Definition

One NDU is the smallest reproducible change in the normalized field state that changes its quantized symbolic coordinate by one integer step.

The NDU is deliberately **not** a physical voltage, force, pressure, IQ score, or unit of human ability. Physical measurements retain their SI units until the explicit quantization boundary.

## Field coordinates

The proposed state is:

`S = (V, |grad V|, theta, U)`

where:

- `V` = measured voltage;
- `|grad V|` = spatial voltage-gradient magnitude;
- `theta` = gradient direction;
- `U` = local uniformity in `[0,1]`.

The encoder maps normalized coordinates to integer NDU coordinates:

`Q(S) = round(N(S) / delta_NDU)`

The NDU is therefore a computational resolution, not an assertion about the physical continuity of the underlying field.

## Strong-unit rule

A strong NDU should satisfy:

1. deterministic results for identical input;
2. monotonic quantization within each coordinate;
3. bounded integer representation;
4. explicit treatment of invalid or missing measurements;
5. independence from language-specific semantic labels;
6. reproducible serialization across machines.

This makes the NDU a useful bridge from continuous field measurements to the discrete digraph/concept layer without claiming that voltage itself contains linguistic meaning.
