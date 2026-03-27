# PROMPT EXPERT - LUMVORAX V6 (Binary Dependency Optimization)

## Context
The goal is to stop compiling C/H files on the fly in Kaggle kernels and switch to a certified binary dependency model (.whl or .so).

## Critical Mission
1. **Source Integrity**: Use ONLY verified sources from `src/`. NO STUBS.
2. **Binary Packaging**:
   - Generate a `.so` (shared object) containing all 39 modules.
   - Wrap it in a Python package `lumvorax`.
   - Produce a `.whl` file compatible with Kaggle's Python version (3.10/3.11).
3. **Kaggle Deployment**:
   - Push the `.whl` and assets to the `nx47-dependencies` dataset.
   - Use the `KAGGLE_KEY` from secrets.
4. **Validation**:
   - The test 360 must now pass by importing the pre-compiled module.
   - `LUMVORAX_REQUIRE_NATIVE=1` must check the presence of the `.so`.

## Execution Commands
`python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/produce_wheels_v4.py`
`kaggle datasets version -p RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle -m "V5: Binary migration"`
