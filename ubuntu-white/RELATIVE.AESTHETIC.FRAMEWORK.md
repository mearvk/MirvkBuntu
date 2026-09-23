# Relative Aesthetic Framework

## Status

This is the provisional mathematical framework for MirvkBuntu desktop color and visual optimization. It defines **relative importance** before exact colors are calculated.

The scale is intentionally 0–100. It is a design-control scale, not a claim that aesthetic quality has a universal objective measurement.

## 1. Relative Importance Vector

Define the design vector:

    I = (C,K,H,M,B,S,E,A,X)

| Variable | Category | Meaning |
|---|---|---|
| C | Clarity | Immediate visual intelligibility |
| K | Contrast | Foreground/background separation |
| H | Hierarchy | Relative visual importance of elements |
| M | Harmony | Compatibility among colors and visual components |
| B | Balance | Distribution of visual weight |
| S | Semantic Consistency | Same meaning, same visual treatment |
| E | Restraint | Control of unnecessary visual excess |
| A | Accessibility | Readability and usable state distinction |
| X | Context Stability | Stability across display/context changes |

Each value is an integer from **0 to 100**.

- `0` = no relative importance assigned
- `25` = low importance
- `50` = ordinary importance
- `75` = high importance
- `100` = maximum importance

These numbers are **inputs to optimization before exact desktop colors are calculated**.

## 2. Weighted Objective

The optimizer may normalize the importance values:

    w_i = I_i / sum(I_j)

and construct a weighted objective:

    J(c) = sum(w_i * R_i(c))

where `c` is a candidate color/system configuration, `R_i(c)` is the achieved 0–100 result for category `i`, and `w_i` is the requested relative importance.

The optimizer searches for a configuration that maximizes `J` subject to hard constraints.

## 3. Hard Constraints

Some requirements are not tradeable merely because another category has a higher weight.

Examples:

    A(c) >= A_min
    K(c) >= K_min
    S(c) >= S_min

> A failed mandatory accessibility or semantic constraint invalidates the candidate, regardless of its aggregate aesthetic objective.

## 4. Input and Output Are Separate

The framework deliberately distinguishes **Importance** from **Achievement**.

### Input

    I = (I_C,I_K,I_H,I_M,I_B,I_S,I_E,I_A,I_X)

This says what the design is being optimized to emphasize.

### Output

    R(c) = (R_C,R_K,R_H,R_M,R_B,R_S,R_E,R_A,R_X)

This says what the generated design actually achieves.

The optimizer therefore has the form:

    O(I, D, P) -> (c, R, J)

where `I` is the importance vector, `D` is desktop/design constraints, `P` is color/perceptual parameters, `c` is the generated configuration, `R` is the achieved category vector, and `J` is the aggregate objective.

## 5. Relative Aesthetic Result

A reporting value may be calculated after optimization:

    A_R = sum(I_i * R_i) / sum(I_i)

`A_R` is on the 0–100 scale.

This is a **relative result for the specified design configuration**, not a universal measurement of beauty.

The report should retain the complete vector rather than only the aggregate:

    (I, R, A_R)

so that a single number never hides a weak category.

## 6. Color Optimization

Exact colors are generated only after the importance vector and hard constraints have been established.

A color candidate may be represented in OKLCH:

    c = (L,C,h)

with an optional contextual coordinate:

    c_4 = (L,C,h,q)

where `q` represents rendering context rather than a fourth physical color channel.

The optimizer can therefore search a 3D color space with an optional 4D context space while preserving semantic roles such as surface, surface-alt, text, text-muted, disabled, primary, primary-pressed, secondary, secondary-hover, border, and on-accent.

## 7. Example MirvkBuntu Starting Vector

A first desktop profile can use:

| Category | Importance |
|---|---:|
| Clarity | 95 |
| Contrast | 95 |
| Hierarchy | 90 |
| Harmony | 88 |
| Balance | 82 |
| Semantic Consistency | 96 |
| Restraint | 90 |
| Accessibility | 100 |
| Context Stability | 80 |

These are **starting inputs**, not calculated results. The values can later be changed by the profile, user configuration, or optimizer.

## 8. Optimizer Feedback

The optimizer should report both requested importance and achieved result:

| Category | Importance | Achieved | Gap |
|---|---:|---:|---:|
| Clarity | I_C | R_C | R_C-I_C |
| Contrast | I_K | R_K | R_K-I_K |
| Hierarchy | I_H | R_H | R_H-I_H |
| Harmony | I_M | R_M | R_M-I_M |
| Balance | I_B | R_B | R_B-I_B |
| Semantic Consistency | I_S | R_S | R_S-I_S |
| Restraint | I_E | R_E | R_E-I_E |
| Accessibility | I_A | R_A | R_A-I_A |
| Context Stability | I_X | R_X | R_X-I_X |

A negative gap does not automatically mean failure. It identifies where the generated design did not reach the requested target and should be considered during the next optimization iteration.

## 9. Iterative Optimization

The intended cycle is:

1. Establish importance values.
2. Establish hard constraints.
3. Generate candidate colors.
4. Calculate perceptual relationships.
5. Validate accessibility and semantic constraints.
6. Calculate achieved category values.
7. Calculate the weighted objective.
8. Adjust candidate parameters.
9. Repeat until the stopping condition is reached.
10. Emit the final color tokens and a reproducible optimization report.

Conceptually:

    I -> O_0 -> R_0 -> O_1 -> R_1 -> ... -> O_n -> R_n

The optimizer should be deterministic when its inputs, configuration, algorithm version, and color-space implementation are identical.

## 10. Excellent / Very High Quality

The existing MirvkBuntu target of **AESTHETIC_GRADE=EXCELLENT** and **QUALITY_TARGET=VERY_HIGH** should mean that:

- mandatory constraints pass;
- semantic roles remain distinguishable;
- the requested importance hierarchy is respected;
- color relationships remain perceptually coherent;
- accessibility requirements are satisfied;
- unnecessary visual complexity is controlled;
- the generated result is reproducible;
- the complete input/output vector is recorded.

"Excellent" therefore describes conformance to the defined framework and quality requirements, rather than asserting a universal objective ranking of aesthetics.

## 11. Design Principle

> **The importance numbers describe what the desktop should value. The optimizer converts those priorities into exact visual parameters. The resulting measurements describe what the generated desktop actually achieved.**

This establishes a clean separation between **design intent**, **optimization**, and **measured result**.