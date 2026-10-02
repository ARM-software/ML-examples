# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

"""Public API for LUT-aware 2-bit quantization."""

from .core import (
    EPS,
    GENERAL_CODEBOOKS,
    SYMMETRIC_R,
    QuantResult,
    _blocks,
    _row_blocks,
    best_symmetric_r,
    bits_per_weight,
    codebook_to_int8,
    demo,
    dequantize,
    emit_zt0,
    fit_affine,
    fit_scaled,
    kmeans_codebook,
    load_input,
    pack_indices,
    quantize,
    recommend,
    report,
    snr_db,
    symmetric_codebook,
    unpack_indices,
    write_c_fixture,
    write_outputs,
)
from .cli import main

__all__ = [
    "EPS",
    "GENERAL_CODEBOOKS",
    "SYMMETRIC_R",
    "QuantResult",
    "best_symmetric_r",
    "bits_per_weight",
    "codebook_to_int8",
    "demo",
    "dequantize",
    "emit_zt0",
    "fit_affine",
    "fit_scaled",
    "kmeans_codebook",
    "load_input",
    "main",
    "pack_indices",
    "quantize",
    "recommend",
    "report",
    "snr_db",
    "symmetric_codebook",
    "unpack_indices",
    "write_c_fixture",
    "write_outputs",
]
