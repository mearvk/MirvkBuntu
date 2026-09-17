# CI Large Sampling Map

The UTF-4088 large sampling experiment is now executable through GitHub Actions.

## Workflow

`.github/workflows/utf4088-large-map.yml`

The workflow:

1. checks out the exact repository revision;
2. compiles `glyph8x12.cpp` and `large_sampling_map.cpp` with C++17 and optimization;
3. runs exactly **100,000,000** samples using the fixed seed in the sampler;
4. validates that the generated CSV contains exactly 100,000,000 data rows;
5. uploads the complete sampling map, density summary, and console result as workflow artifacts;
6. uploads the source files used for the run as a provenance artifact.

## Reproducibility

The workflow pins the experiment to the Git commit that triggered the run. The sampling algorithm and seed are versioned in the repository. Consequently, an artifact can be associated with the exact source revision that generated it.

## Result interpretation

The CI run is the authoritative numerical execution of the 100M experiment. This document deliberately does not embed a numerical result until a workflow run has actually completed successfully. A source commit alone is not evidence that the 100M computation was executed.

## Artifact policy

The generated CSV can be large. It is therefore retained as a GitHub Actions artifact rather than committed to the repository's Git history. The source, methodology, seed, and validation procedure remain permanently versioned in Git.

The summary and console result should be used for routine feasibility review; the complete CSV is available when distribution-level inspection is required.
