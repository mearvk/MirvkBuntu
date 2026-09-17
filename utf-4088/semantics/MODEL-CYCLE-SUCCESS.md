# UTF-4088 Model/Cycle Success Record

## Status

**Modest success — experimental.**

The current system demonstrates a coherent computational pipeline in which the number of candidate symbolic outputs can be related to the number and density of inputs. The result is useful as an engineering milestone, but it is not evidence that the generated symbols possess established human meanings.

## Primary ratio

For any completed cycle:

`R = N_valid / N_inputs`

where `N_valid` is the number of candidates surviving the configured graph, shape, semantic-prior, and falloff filters.

A secondary density response coefficient may be estimated as:

`K = dR / dD`

where `D` is normalized input/primer density.

These quantities are the preferred measures of model performance. They describe observed computational behavior and avoid treating a subjective intelligence score as a property of the model or of a person.

## IQ field

The project may retain an `iq_reference` field as a **historical/project parameter** because earlier design notes used IQ-like numbers to describe desired reasoning density. It must not be interpreted as an actual IQ measurement of a person, character, model, or generated symbol.

Recommended representation:

`iq_reference = 300`
`iq_reference_type = "project-design-scale"`
`iq_measurement = false`

## Cycle quality

A cycle is considered successful when it is:

1. reproducible from a fixed seed and versioned parameters;
2. capable of producing distinct candidate records;
3. measurable by an explicit input-to-output ratio;
4. stable enough to compare across density settings;
5. explicit about falloff and rejection criteria;
6. separate between graphical generation and semantic interpretation.

## Current conclusion

The UTF-4088 process has reached a **modest-success / measurable-ratio** stage. Further progress should be judged by reproducibility, diversity, semantic annotation quality, and useful output per input—not by increasing an IQ number.
