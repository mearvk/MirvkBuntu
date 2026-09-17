# UTF-4088 Gradation Matrix

The gradation matrix is the ordering mechanism for representation quality.

| Dimension | Start | Intermediate | Final |
|---|---|---|---|
| Shape | coarse canonical form | refined geometry | published geometry |
| Meaning | entry concept | constrained concept | registered concept |
| Language | tuple member | cross-language relation | published language mapping |
| Field | initial sample | normalized field | stable quantized field |
| Graph | seed node | weighted path | stable directed relation |
| Rendering | provisional | deterministic | immutable publication |

A symbol may advance only when its representation satisfies the validation rules for the next stage. "Final" therefore means **final within this experimental registry/version**, not metaphysically final or guaranteed to be universally correct.

## Precision

The renderer should retain maximum internal precision until the final quantization step. Floating-point inputs must be normalized and bounded before conversion into published integer identifiers. Repeated identical inputs must produce identical outputs.

## 4D remainder space

The conceptual remainder space is addressed by `(x,y,pressure,voltage)`. The renderer does not claim that these physical variables inherently determine linguistic meaning. Instead, they provide a deterministic address from which an experimental shape can be generated.

The United States / American-English component is represented as one member of the three-language tuple and does not displace Korean or Germanic records.
