# MirvkBuntu Color Inferencer

## Concept

The Color Inferencer is a proposed deterministic subsystem for deriving and validating desktop colors from semantic requirements. It should not guess arbitrary colors; it should infer candidates from constraints.

## Inputs

1. Semantic role.
2. Base surface.
3. Desired perceptual lightness.
4. Chroma range.
5. Hue family.
6. Required contrast.
7. Neighboring colors.
8. UI importance.
9. Display/rendering context.

## Inference sequence

semantic role -> constraints -> perceptual color space -> candidate set -> contrast test -> separation test -> gamut mapping -> final token

## Candidate scoring

Q(c)=w1*K(c)+w2*D(c)+w3*H(c)+w4*G(c)-w5*X(c), where K is contrast quality, D is perceptual distance from conflicting roles, H is hierarchy quality, G is gamut/rendering validity, and X is excessive chroma or visual noise.

Hard constraints are evaluated before candidate scoring.

## 3D mode

Default inference operates in a three-dimensional perceptual color manifold: lightness x chroma x hue.

## 4D mode

An optional fourth coordinate represents context: lightness x chroma x hue x context. Context may represent display brightness, ambient-light class, UI emphasis, accessibility mode, or display gamut. It changes rendering policy, not semantic meaning.

## Purity requirement

Inference is deterministic. Identical source palette, semantic roles, constraints, display profile, and algorithm version should produce the same result.

## Output

The inferencer should produce semantic color tokens, GTK CSS variables, GNOME Shell CSS values, dconf settings, validation reports, contrast/separation tables, and machine-readable color manifests.

## Quality target

Target aesthetic: Excellent. Intended grade: Quality / Very High Quality. Optimize for clarity, mathematical consistency, perceptual stability, and restrained composition rather than maximum saturation.
