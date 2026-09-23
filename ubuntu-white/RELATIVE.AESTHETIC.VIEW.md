# Relative Aesthetic — Design View

## Purpose

MirvkBuntu treats desktop aesthetics as an optimization problem with explicit, configurable priorities.

The **Relative Aesthetic** model does not claim that beauty has one universal numerical value. Instead, it provides a consistent way to describe what a desktop design should emphasize **before exact colors and visual parameters are generated**.

## The 0–100 Importance Scale

Each design category receives a relative importance value from **0 to 100**.

| Range | Interpretation |
|---:|---|
| 0 | No relative importance |
| 1–24 | Very low |
| 25–49 | Low |
| 50–74 | Ordinary to moderate |
| 75–89 | High |
| 90–99 | Very high |
| 100 | Maximum |

The values are **inputs to the optimizer**.

They are not themselves aesthetic measurements.

## Relative Aesthetic Categories

The current framework uses:

- **Clarity** — how readily the interface can be understood.
- **Contrast** — separation between foreground and background elements.
- **Hierarchy** — differentiation of primary, secondary, and subordinate elements.
- **Harmony** — compatibility among colors, typography, surfaces, icons, and controls.
- **Balance** — distribution of visual weight.
- **Semantic Consistency** — consistent visual treatment of equivalent meanings.
- **Restraint** — control of unnecessary decoration and visual noise.
- **Accessibility** — readable and distinguishable states and content.
- **Context Stability** — preservation of the intended system across display and rendering contexts.

## Input Before Color

The important architectural distinction is:

```
Design Priorities
      |
      v
Importance Vector (0–100)
      |
      v
Optimizer
      |
      v
Perceptual Color / Layout Search
      |
      v
Exact Desktop Configuration
```

The system therefore does **not** begin by choosing arbitrary colors and then attempting to justify them.

It begins with design priorities and derives colors from those priorities and the applicable constraints.

## Optimizer Output

The optimizer produces two related vectors.

### Requested Importance

```
I = (I_C, I_K, I_H, I_M, I_B, I_S, I_E, I_A, I_X)
```

This records what the design was asked to value.

### Achieved Result

```
R = (R_C, R_K, R_H, R_M, R_B, R_S, R_E, R_A, R_X)
```

This records what the generated design actually achieves.

The optimizer can then calculate a relative aggregate:

```
A_R = sum(I_i * R_i) / sum(I_i)
```

with a result on the 0–100 scale.

The complete vectors remain more important than the aggregate because an overall number must not hide a weak individual category.

## Hard Constraints

Certain properties are treated as requirements rather than merely weighted preferences.

For example:

```
Accessibility >= minimum
Contrast     >= minimum
Semantic     >= minimum
```

A candidate that fails a mandatory constraint is rejected even if other categories would give it a high aggregate result.

This keeps aesthetic optimization subordinate to usability and functional correctness.

## Color Space

Exact color generation may operate in a perceptual color representation such as OKLCH:

```
Color = (L, C, h)
```

An optional fourth coordinate can represent rendering context:

```
Contextual Color = (L, C, h, q)
```

The fourth coordinate is a **context variable**, not a fourth physical color channel. It can represent factors such as display conditions, UI density, semantic emphasis, or other rendering-policy inputs.

This allows the Color Inferencer to operate in a 3D color space with an optional 4D contextual optimization space.

## Starting MirvkBuntu Profile

The current proposed starting importance vector is:

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

These values are intentionally configurable.

They describe the **initial design philosophy**, not the final measured quality of the desktop.

## Optimization Cycle

The intended process is:

1. Define relative importance.
2. Define hard constraints.
3. Generate candidate visual parameters.
4. Evaluate perceptual relationships.
5. Verify accessibility and semantic requirements.
6. Measure achieved results.
7. Calculate the weighted objective.
8. Adjust the candidate.
9. Repeat until the stopping criteria are met.
10. Publish the resulting colors, configuration, and optimization report.

For reproducibility, identical inputs, configuration, optimizer version, and perceptual implementation should produce the same result.

## Design Principle

> **The importance numbers describe what the desktop should value. The optimizer converts those priorities into exact visual parameters. The resulting measurements describe what the generated desktop actually achieved.**

This creates a deliberate separation between:

**Design Intent → Optimization → Exact Configuration → Measurement**

The framework is therefore suitable for MirvkBuntu's Color Inferencer, GNOME configuration, and Ubuntu-White desktop system.
