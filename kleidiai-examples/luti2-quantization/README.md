<!--
    SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>

    SPDX-License-Identifier: Apache-2.0
-->

# lut2quant

`lut2quant` selects and exports four-entry lookup tables for 2-bit weight
quantisation. It evaluates fixed and data-fitted codebooks against an input
tensor and recommends the lowest-MSE table to share across that tensor.

The [tutorial](TUTORIAL.md) walks through recommendation, interpretation,
export, and deployment checks.

## Installation

`lut2quant` requires Python 3.11 or later and uses
[`uv`](https://docs.astral.sh/uv/getting-started/installation/) for installation.
After installing `uv`, change to this example directory and run:

```text
uv sync
```

If you use an IDE, configure it to use the virtual environment created by
`uv sync`. The environment location and interpreter path can vary by operating
system and IDE.

## Quick start

If you do not already have a tensor file, create a small NumPy example:

```text
uv run python examples/create_example.py
```

Recommend one shared LUT for a NumPy, CSV, or whitespace-delimited tensor:

```text
uv run lut2quant layer.npy --block 256 --out recommendation.json
```

The command prints the recommended LUT, emitted int8 table, improvement over
the arithmetic baseline, and suggested next step. The complete candidate
results are written to `recommendation.json`.

The generated example has shape `[64, 256]`. Using `--block 256` gives each
row (normally one output channel of a weight matrix) its own FP32 weight scale,
matching a channelwise quantisation layout. The LUT itself is still shared by
all rows.

The shared-LUT search compares:

- distribution-matched symmetric tables for uniform, Gaussian, and
  Laplacian weights;
- the `{-2,-1,0,1}` arithmetic grid as a baseline;
- a fitted symmetric inner/outer ratio; and
- a fitted general four-point table.

For deeper analysis, start with `recommended_tensor_lut`, then inspect
`tensor.snr_db` (higher is better), `tensor.mse` (lower is better), and
`tensor.bits_per_weight` (smaller is cheaper). The gain over `int2-grid` shows
whether LUT placement improves on the arithmetic 2-bit baseline for this
tensor. These are reconstruction metrics, not guarantees of model quality.

### Further experiments

The main workflow uses one tensor-wide LUT. The tool also contains exploratory
options that are not part of the existing channelwise qsu2cxp kernel format.

Add `--allow-affine` to consider a scale-plus-offset representation. Affine
results use more metadata and should be compared at their reported storage
cost, not directly with symmetric results.

To measure the potential accuracy from choosing among four LUTs per scale
group:

```text
uv run lut2quant layer.npy --block 32 --compare-lut-scopes --out comparison.json
```

This reports the per-block result but continues to export the shared-LUT
representation. To export the experimental representation explicitly:

```text
uv run lut2quant layer.npy --block 32 --lut-scope block --packed-codes-out weights.bin --selectors-out selectors.bin --out recommendation.json
```

With four candidates, the selector uses two bits per block: `2 / block_size`
bits per weight, or 0.0625 bpw for blocks of 32. This accounts only for stored
metadata. The cost of switching tables in a kernel is platform- and
implementation-dependent and is not benchmarked by this utility.

### Output layout

Blocks run along the tensor's final dimension and never cross row boundaries.
Logical indices written with `--codes-out` retain the input shape. Packed
output includes padding at each row tail; padding is excluded from fitting,
selection, and reported error. The JSON report records the packing order and
whether the shared or per-block representation was exported.

### Optional C integration fixture

`--c-fixture-out fixture.h` emits known reference data for testing a C packer
or Arm® SME2 LUTI2 kernel. It contains the selected int8 LUT, weight scales,
offsets, packed indices, optional selectors, and layout constants. It is an
integration-test fixture, not a production model format or kernel interface.

## Python API

```python
import numpy as np
import lut2quant as lq

weights = np.load("layer.npy")

shared, _, summary = lq.recommend(
    weights,
    block_size=weights.shape[-1],
    include_block=False,
)

lq.report(shared, weights)
reconstructed = lq.dequantize(shared, use_int8_lut=True)
packed = lq.pack_indices(shared.indices)
```

`use_int8_lut=True` reconstructs from the emitted int8 table values and the
per-group weight scales. It does not emulate integer accumulation or
FP32 rounding of scales;
reported MSE and SNR remain reference calculations in NumPy float64.

The lower-level `quantize()` API accepts an explicit candidate set. For a
like-for-like symmetric per-block comparison:

```python
result = lq.quantize(
    weights,
    block_size=32,
    luts=("uniform", "gaussian", "laplacian"),
    granularity="block",
)
```

## Interpreting the report

MSE and SNR measure reconstruction error on the supplied weights. Higher SNR
is better; when comparing the same weights, +3 dB means half the error power.
These metrics are useful for
comparing formats under controlled conditions, but they weight every tensor
position equally. They do not predict model quality by themselves. Any format
decision should ultimately be checked with an end-to-end metric appropriate
to the target model.

`bits_per_weight` is a nominal format cost that includes 2-bit indices,
per-group FP32 metadata, and per-block LUT selectors where applicable. It does
not include container headers, row-tail padding, or the small once-per-tensor
cost of storing a custom LUT. The current reference
implementation stores scales as NumPy floating-point values and emits C
fixtures with `float` arrays; it does not include FP32 scale rounding in its
accuracy score.

## Synthetic demonstration

```text
uv run lut2quant --demo
```

The demonstration compares fixed, selected, and fitted codebooks on several
controlled distributions. It is intended to explain the selection mechanism,
not to establish end-to-end model quality. In its output, `sel-sym` selects
among the `uniform`, `gaussian`, and `laplacian` LUTs, while `sel-all` also
allows the affine candidate. Compare `sel-sym` with the three fixed symmetric
columns at the same storage budget. Affine results may use an extra offset
field and therefore trade more storage for lower error.

## Project layout

| path | purpose |
|---|---|
| `src/lut2quant/` | library and command-line interface |
| `examples/` | portable example-input generator |
| `tests/` | active correctness and CLI tests |
| `tutorial_assets/` | diagrams used by the tutorial |

## Development checks

Run the test suite from the source tree with:

```text
uv run pytest
```

Pytest settings are stored in `pyproject.toml`; only the active suite under
`tests/` is discovered by default.

To verify that the package builds successfully, run:

```text
uv build
```

## Trademarks

- Arm® is a registered trademark of Arm Limited (or its subsidiaries) in the
  US and/or elsewhere.
- All other trademarks are the property of their respective owners.
