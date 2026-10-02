#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

"""
Correctness tests for lut2quant: the tutorial's claims, as executable invariants.

    uv run pytest -v

Each test names the tutorial section whose claim it enforces. If one of these
fails after a change, either the code or the documentation is now wrong.
"""

import json

import numpy as np
import pytest

import lut2quant as lq
from lut2quant import core

RNG = np.random.default_rng(0)
GAUSS = RNG.standard_normal(1 << 14)


# ------------------------------------------------------ shape vs magnitude

def test_scale_invariance_all_magnitude_lives_in_the_scale():
    """Section 4.1: quantising c*w gives identical indices, c*scales, same SNR."""
    qr1 = lq.quantize(GAUSS, 32, granularity="block")
    qr2 = lq.quantize(GAUSS * 37.5, 32, granularity="block")
    assert np.array_equal(qr1.indices, qr2.indices)
    assert np.array_equal(qr1.lut_ids, qr2.lut_ids)
    assert np.allclose(qr2.scales, qr1.scales * 37.5, rtol=1e-10)
    s1 = lq.snr_db(GAUSS, lq.dequantize(qr1))
    s2 = lq.snr_db(GAUSS * 37.5, lq.dequantize(qr2))
    assert abs(s1 - s2) < 1e-8


def test_codebook_overall_scaling_is_absorbed_by_weight_scale():
    """Section 4.1 substitution test: {-1,-r,r,1} and 2x{-1,-r,r,1} reconstruct identically."""
    code = lq.symmetric_codebook(0.3)
    x = GAUSS[:4096]
    qa = lq.quantize(x, 32, luts=(), extra_codebooks={"a": code},
                     granularity="tensor")
    qb = lq.quantize(x, 32, luts=(), extra_codebooks={"b": 2.0 * code},
                     granularity="tensor")
    assert np.array_equal(qa.indices, qb.indices)
    assert np.allclose(lq.dequantize(qa), lq.dequantize(qb), rtol=1e-10)


# ------------------------------------------------------------ scale fitting

def test_refit_never_worse_than_absmax():
    """Section 3.2: the least-squares fit improves on absmax scaling."""
    w, _ = lq._blocks(GAUSS, 32)
    code = lq.symmetric_codebook(0.3)
    s0 = np.max(np.abs(w), axis=1, keepdims=True)  # absmax (max|code| = 1)
    idx0 = np.argmin(np.abs(w[:, :, None] - s0[:, :, None] * code), axis=2)
    mse0 = np.mean((w - s0 * code[idx0]) ** 2, axis=1)
    _, _, mse = lq.fit_scaled(w, code, iters=3)
    assert np.all(mse <= mse0 + 1e-12)


def test_more_lloyd_iterations_never_hurt():
    w, _ = lq._blocks(GAUSS, 32)
    code = lq.symmetric_codebook(0.3)
    _, _, m1 = lq.fit_scaled(w, code, iters=1)
    _, _, m4 = lq.fit_scaled(w, code, iters=4)
    assert m4.mean() <= m1.mean() + 1e-12


def test_scales_always_positive():
    for x in (GAUSS, np.zeros(1024), np.full(1024, -3.0)):
        qr = lq.quantize(x, 32)
        assert np.all(qr.scales > 0)
        assert np.all(np.isfinite(lq.dequantize(qr)))


# --------------------------------------------------------------- selection

def test_selection_never_loses_to_any_single_candidate():
    """Section 3.3: per-block argmin implies the mix beats every column."""
    qr = lq.quantize(GAUSS, 32, granularity="block")
    assert qr.mse <= min(qr.per_lut_mse.values()) + 1e-12


def test_larger_candidate_set_never_loses():
    sym = lq.quantize(GAUSS, 32, luts=("uniform", "gaussian", "laplacian"),
                      granularity="block")
    allc = lq.quantize(GAUSS, 32, granularity="block")
    assert allc.mse <= sym.mse + 1e-12


def test_per_block_never_loses_to_per_tensor():
    t = lq.quantize(GAUSS, 32, granularity="tensor")
    b = lq.quantize(GAUSS, 32, granularity="block")
    assert b.mse <= t.mse + 1e-12


# ------------------------------------------------------------- int8 / ZT0

def test_int8_path_within_rounding_of_float_path():
    """Section 4.1 / 5: table rounding is immaterial (<= 0.05 dB)."""
    qr = lq.quantize(GAUSS, 32)
    d = abs(lq.snr_db(GAUSS, lq.dequantize(qr))
            - lq.snr_db(GAUSS, lq.dequantize(qr, use_int8_lut=True)))
    assert d < 0.05


def test_documented_zt0_tables():
    """Section 4.1: the published int8 tables."""
    expect = {"uniform": [-127, -42, 42, 127],
              "gaussian": [-127, -38, 38, 127],
              "laplacian": [-127, -29, 29, 127]}
    for name, vals in expect.items():
        lut, lut_scale = lq.codebook_to_int8(
            lq.symmetric_codebook(lq.SYMMETRIC_R[name]))
        assert list(lut) == vals
        assert abs(lut_scale - 1 / 127) < 1e-12
    lut, lut_scale = lq.codebook_to_int8(lq.GENERAL_CODEBOOKS["int2-grid"])
    assert (list(lut) == [-127, -64, 0, 64]
            and abs(lut_scale - 2 / 127) < 1e-12)


# ------------------------------------------------- the worked example (2.1)

def test_worked_example_pinned():
    """Sections 2.1 / C.4 and the deck: docs and code must not drift."""
    w = np.array([[0.31, -1.22, 0.07, 0.89, -0.44, 1.65, -0.10, -0.73]])
    idx, s, mse = lq.fit_scaled(w, lq.symmetric_codebook(0.2998), iters=3)
    assert list(idx[0]) == [2, 0, 2, 2, 1, 3, 1, 1]
    assert abs(s[0] - 1.4301) < 5e-4
    assert abs(mse[0] - 0.0809) < 5e-4
    packed = lq.pack_indices(idx)
    assert [hex(b) for b in packed] == ["0xa2", "0x5d"]


# --------------------------------------------------------- theory recovery

def test_kmeans_recovers_lloyd_max_ratio_on_gaussian():
    """Section 3.5: k-means rediscovers r ~ 0.30 on Gaussian data."""
    blocks, _ = lq._blocks(GAUSS, 32)
    cb = lq.kmeans_codebook(blocks)
    r = abs(cb[1]) / abs(cb[0])
    assert 0.26 < r < 0.34


def test_opt_r_recovers_lloyd_max_on_gaussian():
    blocks, _ = lq._blocks(GAUSS, 32)
    r = lq.best_symmetric_r(blocks)
    assert 0.26 < r < 0.36


# ----------------------------------------------------------------- affine

def test_affine_exact_on_four_level_data():
    """Section 3.4: a block that IS {m, m+s, m+2s, m+3s} quantises losslessly."""
    x = np.tile([0.5, 1.0, 1.5, 2.0], 8)
    qr = lq.quantize(x, 32, luts=("affine",), granularity="tensor")
    assert np.max(np.abs(lq.dequantize(qr) - x)) < 1e-9


def test_symmetric_exact_on_matching_levels():
    r = 0.3
    x = np.tile([-1.7, -1.7 * r, 1.7 * r, 1.7], 8)
    qr = lq.quantize(x, 32, luts=("gaussian",), granularity="tensor")
    # gaussian r = 0.2998 vs 0.3: near-exact, dominated by the tiny r mismatch
    assert lq.snr_db(x, lq.dequantize(qr)) > 55.0


# --------------------------------------------------------- packing / shapes

def test_pack_unpack_roundtrip():
    idx = RNG.integers(0, 4, size=4096).astype(np.uint8)
    assert np.array_equal(lq.unpack_indices(lq.pack_indices(idx), idx.size),
                          idx)
    assert np.array_equal(
        lq.unpack_indices(lq.pack_indices(idx, "msb-first"), idx.size,
                          "msb-first"), idx)


def test_non_multiple_of_block_and_nd_shapes():
    for shape in [(100,), (7, 33), (5, 6, 7)]:
        x = RNG.standard_normal(shape)
        qr = lq.quantize(x, 32)
        y = lq.dequantize(qr)
        assert y.shape == x.shape
        assert np.all(np.isfinite(y))


def test_row_tails_are_masked_and_blocks_do_not_cross_rows():
    x = np.array([[8.0, 7.0, 6.0], [0.3, 0.2, 0.1]])
    together = lq.quantize(x, 4, luts=("gaussian",), granularity="tensor")
    first = lq.quantize(x[0], 4, luts=("gaussian",), granularity="tensor")
    second = lq.quantize(x[1], 4, luts=("gaussian",), granularity="tensor")
    assert np.allclose(together.scales, [first.scales[0], second.scales[0]])
    assert np.array_equal(together.indices[0, :3], first.indices[0, :3])
    assert np.array_equal(together.indices[1, :3], second.indices[0, :3])
    expected = np.mean((x - lq.dequantize(together, True)) ** 2)
    assert abs(together.mse - expected) < 1e-12


def test_invalid_quantize_arguments():
    with pytest.raises(ValueError, match="empty"):
        lq.quantize(np.array([]), 32)
    with pytest.raises(ValueError, match="NaN or Inf"):
        lq.quantize(np.array([1.0, np.nan]), 32)
    with pytest.raises(ValueError, match="positive"):
        lq.quantize(np.ones(4), 0)
    with pytest.raises(ValueError, match="granularity"):
        lq.quantize(np.ones(4), 4, granularity="row")
    extras = {f"x{i}": np.arange(4.0) for i in range(5)}
    with pytest.raises(ValueError, match="at most four"):
        lq.quantize(np.ones(4), 4, luts=(), extra_codebooks=extras,
                    granularity="block")


def test_bits_per_weight_accounting():
    """Appendix B constants."""
    g = lq.quantize(GAUSS, 32, luts=("gaussian",), granularity="tensor")
    assert abs(lq.bits_per_weight(g) - 3.0) < 1e-9
    s = lq.quantize(GAUSS, 32, luts=("uniform", "gaussian", "laplacian"),
                    granularity="block")
    assert abs(lq.bits_per_weight(s) - 3.0625) < 1e-9


def test_determinism():
    a = lq.quantize(GAUSS, 32)
    b = lq.quantize(GAUSS, 32)
    assert np.array_equal(a.indices, b.indices)
    assert np.array_equal(a.scales, b.scales)


# ----------------------------------------------------- regression floor (5)

def test_snr_regression_floor_on_gaussian():
    """A coarse regression floor: block-32 selection on N(0,1) sits ~9.5 dB."""
    qr = lq.quantize(GAUSS, 32, luts=("uniform", "gaussian", "laplacian"),
                     granularity="block")
    assert lq.snr_db(GAUSS, lq.dequantize(qr)) > 9.2


def test_demo_blockmix_alternates_complete_source_blocks():
    block = 8
    tensors = core._demo_tensors(32, np.random.default_rng(12), block)
    sources = ("gaussian", "laplacian", "uniform", "skewed")
    mixed = tensors["blockmix"].reshape(-1, block)
    for i, source in enumerate(sources):
        expected = tensors[source].reshape(-1, block)[i]
        assert np.array_equal(mixed[i], expected)


def test_demo_describes_non_default_selection_scope(capsys):
    core.demo(block_size=64, n=64, seed=0, granularity="block")
    block_output = capsys.readouterr().out
    assert "each block of 64 picks" in block_output
    assert "0.0312 bpw" in block_output
    assert "each block of 32 picks" not in block_output

    core.demo(block_size=8, n=32, seed=0, granularity="tensor")
    tensor_output = capsys.readouterr().out
    assert "one symmetric table for the whole tensor" in tensor_output
    assert "Tensor selection stores no per-block selector" in tensor_output


def test_demo_fitted_codebooks_ignore_row_tail_padding(monkeypatch, capsys):
    valid_counts = []
    original_best_r = core.best_symmetric_r
    original_kmeans = core.kmeans_codebook

    def checked_best_r(w, *args, valid=None, **kwargs):
        valid_counts.append(int(valid.sum()))
        return original_best_r(w, *args, valid=valid, **kwargs)

    def checked_kmeans(w, *args, valid=None, **kwargs):
        valid_counts.append(int(valid.sum()))
        return original_kmeans(w, *args, valid=valid, **kwargs)

    monkeypatch.setattr(core, "best_symmetric_r", checked_best_r)
    monkeypatch.setattr(core, "kmeans_codebook", checked_kmeans)
    core.demo(block_size=8, n=33, seed=0)
    capsys.readouterr()
    assert valid_counts == [33, 33] * 6


def test_recommendation_and_cli_exports(tmp_path):
    x = RNG.standard_normal((2, 17))
    tensor, block, payload = lq.recommend(x, 8, include_block=True)
    assert payload["recommended_tensor_lut"] in tensor.lut_names
    assert block is not None
    assert payload["experimental_block_switching"][
        "selector_bits_per_weight"] == 0.25

    input_path = tmp_path / "weights.npy"
    report_path = tmp_path / "report.json"
    codes_path = tmp_path / "codes.npy"
    packed_path = tmp_path / "codes.bin"
    selectors_path = tmp_path / "selectors.bin"
    fixture_path = tmp_path / "fixture.h"
    np.save(input_path, x)
    rc = lq.main([
        str(input_path), "--block", "8", "--lut-scope", "block",
        "--compare-lut-scopes", "--out", str(report_path),
        "--codes-out", str(codes_path), "--packed-codes-out", str(packed_path),
        "--selectors-out", str(selectors_path), "--c-fixture-out",
        str(fixture_path),
    ])
    assert rc == 0
    assert json.loads(report_path.read_text())["exported_scope"] == "block"
    assert np.load(codes_path).shape == x.shape
    assert packed_path.stat().st_size == 12
    assert selectors_path.stat().st_size == 2
    fixture = fixture_path.read_text()
    assert "LUT2Q_BLOCK_COUNT 6" in fixture
    assert "lut2q_lut_scales" not in fixture
    assert "lut2q_block_scales" in fixture
    assert "lut2q_block_offsets" in fixture
    assert "fold" not in fixture


def test_cli_prints_concise_summary_when_json_is_written(tmp_path, capsys):
    input_path = tmp_path / "weights.npy"
    report_path = tmp_path / "report.json"
    np.save(input_path, RNG.standard_normal((2, 16)))

    rc = lq.main([str(input_path), "--out", str(report_path)])

    assert rc == 0
    output = capsys.readouterr().out
    assert "Recommended LUT:" in output
    assert "LUTI2 table (int8):" in output
    assert "Scale-group size:" in output
    assert "Improvement:" in output
    assert f"Detailed results written to {report_path}" in output
    assert report_path.is_file()
    report = json.loads(report_path.read_text())
    selected = report["recommended_tensor_lut"]
    assert "lut_scale" not in report["tensor"]["int8_luts"][selected]
    assert "fold" not in report["tensor"]["int8_luts"][selected]


def test_cli_stdout_remains_json_without_out_file(tmp_path, capsys):
    input_path = tmp_path / "weights.npy"
    np.save(input_path, RNG.standard_normal((2, 16)))

    rc = lq.main([str(input_path)])

    assert rc == 0
    payload = json.loads(capsys.readouterr().out)
    assert payload["recommended_tensor_lut"]


if __name__ == "__main__":
    import sys
    sys.exit(pytest.main([__file__, "-v"]))
