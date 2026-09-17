# UTF-4088 Two-Dimensional Voltage Input API

## Status

Experimental interface specification. This document defines a **software representation of an electrical input field**; it does not prescribe a motherboard voltage level, connector pinout, analog front end, or electrical safety boundary.

## 1. Purpose

The UTF-4088 backend may represent an external electrical condition as a two-dimensional input field and transform that field into a deterministic symbolic state:

`V(x,y,t) -> S -> G -> C`

where:

- `V` is a sampled voltage field;
- `S` is the normalized input state;
- `G` is the derived structural/concept graph;
- `C` is the resulting UTF-4088 conceptual symbol/code-point state.

The API is deliberately separated from physical electronics. A hardware adapter is responsible for converting real electrical measurements into validated digital samples.

## 2. Input field

A field sample is represented as:

```cpp
struct VoltageSample2D {
    double x;
    double y;
    double volts;
    double time_seconds;
};
```

A collection of samples constitutes the input field:

```cpp
struct VoltageField2D {
    std::vector<VoltageSample2D> samples;
    double reference_volts;
    double full_scale_volts;
};
```

`x` and `y` are normalized spatial coordinates unless a hardware adapter explicitly documents another coordinate system. `volts` is a measured quantity, not a character identifier.

## 3. Normalization

For a declared full-scale range `[Vmin,Vmax]`, the normalized field value is:

`u(x,y,t) = clamp((V(x,y,t) - Vmin) / (Vmax - Vmin), 0, 1)`

The normalization step prevents downstream language and symbol selection from depending on arbitrary hardware voltage scales.

## 4. Pressure/force abstraction

A backend may derive pressure-like and directional quantities from the normalized field, but these are computational features rather than assertions that voltage itself is pressure or force.

A simple discrete gradient is:

`grad(u) = (du/dx, du/dy)`

with magnitude:

`M = sqrt((du/dx)^2 + (du/dy)^2)`

and direction:

`theta = atan2(du/dy, du/dx)`

These derivatives provide the requested directional “movement” layer while keeping the physical quantities distinct.

## 5. Uniformity

A uniform-field score can be defined from field variance:

`U = 1 / (1 + variance(u))`

Higher `U` indicates a more uniform normalized field. The value is a software metric and should not be interpreted as a physical law.

## 6. Deterministic symbol pipeline

The proposed backend pipeline is:

`voltage field -> normalization -> derivatives -> structural features -> digraph splitter -> concept graph -> symbol selector`

For identical validated input fields, a conforming implementation should return the same structural result and symbol selection.

## 7. Four-dimensional backing

The conceptual state space is four-dimensional:

`(x, y, t, s)`

where `x,y` describe the two-dimensional input field, `t` describes temporal evolution, and `s` is the derived symbolic/semantic state. The implementation may store these dimensions separately; “4D” is an architectural model, not a claim that the host hardware must expose four physical dimensions.

## 8. API contract

A minimal interface is:

```cpp
struct SymbolState {
    std::uint64_t code_point;
    double confidence;
    double direction;
    double uniformity;
};

std::optional<SymbolState>
derive_symbol_from_voltage_field(const VoltageField2D& field);
```

Required properties:

1. Input validation precedes interpretation.
2. Out-of-range or malformed electrical samples are rejected.
3. Physical safety limits belong to the hardware interface, not the symbol encoder.
4. Symbol mappings are deterministic and versioned.
5. Language-family mappings are treated as data/configuration, not as inherent properties of voltage.
6. Historical shape, digraph, and concept layers remain separable from the electrical-input layer.

## 9. Engineering boundary

An engineer may reasonably model a validated sensor field in terms of magnitude, direction, gradients, and uniformity. It is **not** reasonable to infer that a particular language meaning is physically contained in a voltage value. The UTF-4088 design therefore makes the chain explicit:

`measurement -> mathematical feature -> structural representation -> linguistic mapping -> symbol`.

This separation allows the experimental semantic model to evolve without making unsupported physical claims.
