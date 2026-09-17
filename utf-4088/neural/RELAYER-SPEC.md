# UTF-4088 4D Neural Relayer

The relayer defines the next stage after the existing field and graph layers. It is a deterministic synthesis interface, not a claim that a neural network can infer intrinsic meaning from voltage or pressure.

## Pipeline

`4D input field -> pressure application -> feature tensor -> relayer -> expansion engram -> graph equation -> candidate distribution -> character selection`

The 4D field is represented as:

`X = (x, y, p, v)`

where `x,y` are spatial coordinates and `p,v` are normalized pressure and voltage features.

## Relayer

For a reproducible model:

`H = f_theta(X, G)`

where `G` is the existing directed graph representation and `f_theta` is a versioned neural network. Model weights are part of the model identity.

The relayer may synthesize intermediate representations, but the final character is selected by a deterministic decoder:

`P(c|X,G) = softmax(D(H,G))`

`c* = argmax_c P(c|X,G)`

with deterministic tie-breaking by published integer ID.

## Expansion engram

The expansion engram is the persisted feature state used to continue a graph equation across successive inputs. It contains only versioned numerical state and graph references. It must be serializable and hashable.

A recurrent update is:

`E_(t+1) = R(E_t, H_t, G_t)`

The implementation must define `R` explicitly; the term “engram” is an architectural name, not a biological claim.

## Uniformity and idempotence

The system is idempotent when the same canonical input, model version, engram state, and registry version produce the same output:

`F(F(X)) = F(X)`

for the canonicalized representation domain.

This should be enforced by canonicalization, stable quantization, deterministic inference, and deterministic tie-breaking. Ordinary stochastic neural-network inference is not sufficient by itself.

## Character path

A selected character also records the winning graph path and score vector. Thus the result is:

`{character_id, probability_vector, graph_path, equation_state, model_hash}`

The existing historical glyphs and 8x12 candidate symbols remain samples for the graph/resolution stage. They are not silently converted into claims of historical authenticity.
