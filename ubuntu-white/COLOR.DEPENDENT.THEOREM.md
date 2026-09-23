# MirvkBuntu Color-Dependent Theorem

## 1. Philosophy

MirvkBuntu treats color as a measurable component of the desktop interface rather than decoration applied after layout. A color choice is evaluated by role, contrast, luminance, chromatic distance, semantic meaning, and stability across display conditions.

The objective is a mathematical and pure visual experience. Surfaces establish an ordered spatial field; text remains highly legible; accents communicate state without overwhelming the interface; adjacent states have measurable separation; and repeated components preserve the same color grammar.

The target aesthetic grade is Quality / Very High Quality. The system should prefer measurable consistency over arbitrary ornament.

## 2. Color space

The color engine may reason in a perceptual space such as OKLCH rather than treating RGB channel distance as perceptual distance.

Represent a color as C=(L,C,h), where L is perceptual lightness, C is chroma, and h is hue.

For colors a and b, define a configurable perceptual separation D(a,b)=sqrt(wL*(deltaL)^2+wC*(deltaC)^2+wH*(deltaH)^2). The weights are configurable. This is a design model, not a claim that one metric is universally perceptually exact.

## 3. Color-dependent chart

A color-dependent chart is a mapping F:S x R -> C, where S is semantic state, R is rendering role, and C is valid display color.

Example states are surface, text, muted, focus, active, link, warning, error, and success.

The mapping should satisfy role uniqueness, contrast constraints, ordered hierarchy, continuity of related states, bounded chroma, and theme invariance.

## 4. Three-dimensional and four-dimensional extensions

A three-dimensional color manifold uses (L,C,h). An optional fourth coordinate may represent controlled rendering context such as display environment, UI density, semantic emphasis, ambient light, or accessibility mode.

The fourth dimension is not an additional physical color channel. It is a parameter used to optimize rendering policy while preserving semantic meaning.

A configurable objective may be expressed as J = alpha*A + beta*K + gamma*H + delta*S - epsilon*E, where A is accessibility/contrast quality, K is perceptual separation, H is hierarchy clarity, S is semantic consistency, and E is visual excess.

## 5. Purity rules

- White is the principal surface field.
- Near-black is the principal writing field.
- A primary accent communicates active/selected state.
- A secondary accent communicates focus, navigation, or informational emphasis.
- Disabled states reduce contrast without changing semantic identity.
- Decorative gradients are avoided unless they communicate a measurable state.
- Shadows are restrained.
- Borders remain subordinate to surface and text.
- Motion is not required to understand color meaning.
- Color is not the only channel for critical state; text, shape, iconography, or position should reinforce it.

## 6. Aesthetic grading

The design target is Quality / Very High Quality. The system reaches Excellent only when semantic colors are consistent, foreground/background relationships are verified, focus states remain visible, selected states remain distinguishable, GTK and GNOME Shell are coherent, and color transformations preserve hierarchy.

## 7. Implementation rule

The color profile is authoritative for MirvkBuntu's own theme. Applications may retain their own internal palettes, but MirvkBuntu-controlled shell, GTK, notifications, dialogs, menus, and system UI should consume the semantic color contract.

This theorem is a design and engineering framework, not a claim that aesthetic quality can be reduced to one scalar number.
