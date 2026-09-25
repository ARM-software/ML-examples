#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

"""
lut2quant -- 2-bit LUT quantisation with codebook (LUT) selection.

Quantises a float tensor to 2-bit indices plus weight scales, choosing per
block (or per tensor) the 4-entry codebook that minimises MSE. Codebooks are
emitted as int8 LUTs ready for SME2 LUTI2-style expansion (4 bytes at the
bottom of ZT0).

Key ideas
---------
1. Every *symmetric* 4-point codebook is fully described by one number: the
   inner/outer level ratio r, giving levels {-1, -r, +r, +1} x block_scale.
   Distribution-matched (Lloyd-Max) ratios:
       uniform    r = 1/3      levels +-0.25,  +-0.75  (on [-1, 1])
       gaussian   r = 0.2998   levels +-0.4528, +-1.5104 (unit variance)
       laplacian  r = 0.2288   levels +-0.4196, +-1.8340 (unit variance)
2. Given a codebook shape, the optimal scale for each scale group has a closed
   form:
       s = sum(w_i * q_i) / sum(q_i^2)
   where q_i is the assigned (unscaled) codebook value. Alternating
   assignment and scale refit is a Lloyd iteration with a tied codebook
   shape; 2-3 iterations converge.
3. An affine codebook {0,1,2,3} with per-block (scale, offset) handles skewed
   blocks and exactly represents data already lying on an affine 4-point grid.
4. Per-block LUT selection stores a 2-bit LUT id per block (0.0625 bpw at
   block_size=32). A shared per-tensor LUT needs no per-block selector.

Usage
-----
    import lut2quant as lq
    qr = lq.quantize(w, block_size=32, granularity="block")
    w_hat = lq.dequantize(qr)
    lq.report(qr)

    lut2quant --demo            # synthetic-distribution comparison
    lut2quant --demo --block 64 --granularity tensor
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np

EPS = 1e-12

# --------------------------------------------------------------- codebooks

SYMMETRIC_R = {
    "uniform": 1.0 / 3.0,   # Lloyd-Max for uniform distribution
    "gaussian": 0.2998,     # Lloyd-Max for N(0,1)
    "laplacian": 0.2288,    # Lloyd-Max for unit-variance Laplacian
}


# Named general codebooks. "int2-grid" is the two's-complement arithmetic
# grid used as a baseline here and by qsu2-style formats.
GENERAL_CODEBOOKS = {
    "int2-grid": np.array([-2.0, -1.0, 0.0, 1.0]),
}


def symmetric_codebook(r: float) -> np.ndarray:
    """Levels {-1, -r, +r, +1}; scaled per block at fit time."""
    return np.array([-1.0, -r, r, 1.0], dtype=np.float64)


def codebook_to_int8(code: np.ndarray) -> tuple[np.ndarray, float]:
    """Quantise a codebook to int8 for ZT0.

    Returns the integer table and the normalisation used to derive it. Quantisation
    fits the stored weight scale directly against the integer table, so this
    normalisation is not required by the decoder.
    """
    m = float(np.max(np.abs(code)))
    if m < EPS:
        return np.zeros(4, dtype=np.int8), 1.0
    lut = np.clip(np.round(code / m * 127.0), -128, 127).astype(np.int8)
    return lut, m / 127.0


# ------------------------------------------------------------------ fitting

def _blocks(x: np.ndarray, block_size: int) -> tuple[np.ndarray, int]:
    """Compatibility helper: flatten *x* and pad it into fixed-size blocks."""
    if block_size <= 0:
        raise ValueError("block_size must be a positive integer")
    flat = np.asarray(x, dtype=np.float64).reshape(-1)
    n = flat.size
    pad = (-n) % block_size
    if pad:
        flat = np.concatenate([flat, np.zeros(pad)])
    return flat.reshape(-1, block_size), n


def _row_blocks(x: np.ndarray, block_size: int) -> tuple[np.ndarray, np.ndarray]:
    """Block the final dimension without allowing blocks to cross rows."""
    if block_size <= 0:
        raise ValueError("block_size must be a positive integer")
    a = np.asarray(x, dtype=np.float64)
    rows = a.reshape(-1, a.shape[-1]) if a.ndim > 1 else a.reshape(1, -1)
    groups = (rows.shape[1] + block_size - 1) // block_size
    blocks = np.zeros((rows.shape[0], groups, block_size), dtype=np.float64)
    valid = np.zeros_like(blocks, dtype=bool)
    for group in range(groups):
        start = group * block_size
        end = min(start + block_size, rows.shape[1])
        blocks[:, group, : end - start] = rows[:, start:end]
        valid[:, group, : end - start] = True
    return blocks.reshape(-1, block_size), valid.reshape(-1, block_size)


def _valid_mask(w: np.ndarray, valid: np.ndarray | None) -> np.ndarray:
    if valid is None:
        return np.ones_like(w, dtype=bool)
    valid = np.asarray(valid, dtype=bool)
    if valid.shape != w.shape:
        raise ValueError("valid mask must have the same shape as the blocks")
    if np.any(valid.sum(axis=1) == 0):
        raise ValueError("every block must contain at least one valid value")
    return valid


def fit_scaled(w: np.ndarray, code: np.ndarray, iters: int = 3,
               valid: np.ndarray | None = None):
    """Fit per-block positive scale for a fixed codebook shape.

    w: (B, N) blocks. Returns (idx uint8 (B,N), scale (B,), mse (B,)).
    """
    w = np.asarray(w, dtype=np.float64)
    mask = _valid_mask(w, valid)
    code = np.asarray(code, dtype=np.float64)
    if code.shape != (4,) or not np.all(np.isfinite(code)):
        raise ValueError("codebook must contain four finite values")
    cmax = max(float(np.max(np.abs(code))), EPS)
    s = np.max(np.where(mask, np.abs(w), 0.0), axis=1, keepdims=True) / cmax
    s = np.maximum(s, EPS)
    for _ in range(iters):
        idx = np.argmin(np.abs(w[:, :, None] - s[:, :, None] * code), axis=2)
        q = code[idx]
        num = np.sum(np.where(mask, w * q, 0.0), axis=1, keepdims=True)
        den = np.sum(np.where(mask, q * q, 0.0), axis=1, keepdims=True)
        s = np.where(den > EPS, num / np.maximum(den, EPS), s)
        s = np.maximum(s, EPS)                            # keep positive
    idx = np.argmin(np.abs(w[:, :, None] - s[:, :, None] * code), axis=2)
    recon = s * code[idx]
    mse = np.sum(np.where(mask, (w - recon) ** 2, 0.0), axis=1) / mask.sum(axis=1)
    return idx.astype(np.uint8), s[:, 0], mse


def fit_affine(w: np.ndarray, iters: int = 3, valid: np.ndarray | None = None,
               code: np.ndarray | None = None):
    """Affine codebook {0,1,2,3}: w ~= s*q + m per block.

    Returns (idx uint8, scale (B,), min (B,), mse (B,)).
    """
    w = np.asarray(w, dtype=np.float64)
    mask = _valid_mask(w, valid)
    code = np.arange(4.0) if code is None else np.asarray(code, dtype=np.float64)
    if code.shape != (4,) or not np.all(np.diff(code) > 0):
        raise ValueError("affine codebook must contain four increasing values")
    wmin = np.min(np.where(mask, w, np.inf), axis=1, keepdims=True)
    wmax = np.max(np.where(mask, w, -np.inf), axis=1, keepdims=True)
    s = np.maximum((wmax - wmin) / (code[-1] - code[0]), EPS)
    m = wmin
    for _ in range(iters):
        idx = np.argmin(np.abs(w[:, :, None] -
                              (m[:, :, None] + s[:, :, None] * code)), axis=2)
        q = code[idx]
        count = mask.sum(axis=1, keepdims=True)
        qm = np.sum(np.where(mask, q, 0.0), axis=1, keepdims=True) / count
        wm = np.sum(np.where(mask, w, 0.0), axis=1, keepdims=True) / count
        var = np.sum(np.where(mask, (q - qm) ** 2, 0.0), axis=1, keepdims=True)
        cov = np.sum(np.where(mask, (q - qm) * (w - wm), 0.0), axis=1, keepdims=True)
        s = np.where(var > EPS, cov / np.maximum(var, EPS), s)
        s = np.maximum(s, EPS)
        m = wm - s * qm
    idx = np.argmin(np.abs(w[:, :, None] -
                          (m[:, :, None] + s[:, :, None] * code)), axis=2)
    recon = s * code[idx] + m
    mse = np.sum(np.where(mask, (w - recon) ** 2, 0.0), axis=1) / mask.sum(axis=1)
    return idx.astype(np.uint8), s[:, 0], m[:, 0], mse


def kmeans_codebook(w: np.ndarray, iters: int = 25, seed: int = 0,
                    valid: np.ndarray | None = None) -> np.ndarray:
    """Data-driven shared 4-point codebook, fit on absmax-normalised blocks.

    A general (possibly asymmetric) codebook shape shared across the tensor;
    per-block scale is fitted separately with fit_scaled.
    """
    mask = _valid_mask(w, valid)
    absmax = np.maximum(np.max(np.where(mask, np.abs(w), 0.0),
                              axis=1, keepdims=True), EPS)
    x = (w / absmax)[mask]
    if x.size > 200_000:
        rng = np.random.default_rng(seed)
        x = rng.choice(x, 200_000, replace=False)
    c = symmetric_codebook(SYMMETRIC_R["gaussian"]).copy()
    for _ in range(iters):
        a = np.argmin(np.abs(x[:, None] - c), axis=1)
        for k in range(4):
            sel = x[a == k]
            if sel.size:
                c[k] = sel.mean()
    c = np.sort(c)

    # Jointly refine the shared shape and per-block scales. For fixed scales
    # and assignments, each codebook level has a closed-form least-squares
    # update. Normalisation removes the shape/scale ambiguity.
    for _ in range(8):
        idx, scales, _ = fit_scaled(w, c, iters=3, valid=mask)
        updated = c.copy()
        for k in range(4):
            selected = mask & (idx == k)
            if np.any(selected):
                s = np.broadcast_to(scales[:, None], w.shape)
                denom = np.sum((s[selected]) ** 2)
                if denom > EPS:
                    updated[k] = np.sum(s[selected] * w[selected]) / denom
        updated = np.sort(updated)
        magnitude = max(float(np.max(np.abs(updated))), EPS)
        updated /= magnitude
        if np.allclose(updated, c, rtol=1e-7, atol=1e-9):
            c = updated
            break
        c = updated
    return c


def best_symmetric_r(w: np.ndarray, grid: np.ndarray | None = None,
                     iters: int = 2, valid: np.ndarray | None = None) -> float:
    """Grid search the single best inner/outer ratio for this tensor."""
    if grid is None:
        grid = np.linspace(0.05, 0.95, 46)
    best_r, best_mse = grid[0], np.inf
    for r in grid:
        _, _, mse = fit_scaled(w, symmetric_codebook(float(r)), iters=iters,
                               valid=valid)
        counts = _valid_mask(w, valid).sum(axis=1)
        t = float(np.sum(mse * counts) / counts.sum())
        if t < best_mse:
            best_r, best_mse = float(r), t
    return best_r


# ----------------------------------------------------------------- quantise

@dataclass
class QuantResult:
    shape: tuple
    numel: int
    block_size: int
    granularity: str
    lut_names: list
    codebooks: dict          # name -> float codebook (None for affine)
    luts_int8: dict          # name -> int8 lut[4]
    lut_ids: np.ndarray      # (B,) uint8 index into lut_names
    indices: np.ndarray      # (B, N) uint8 in [0, 3]
    scales: np.ndarray       # (B,) float64 weight scales
    mins: np.ndarray         # (B,) float64 block offsets (zero if symmetric)
    mse: float
    per_lut_mse: dict        # name -> tensor MSE if that LUT were used alone
    selection_hist: dict     # name -> fraction of blocks using it
    valid: np.ndarray        # (B, N) true for real values, false for padding
    candidate_mse: np.ndarray  # (C, B), scored through emitted int8 LUTs


def quantize(x, block_size: int = 32,
             luts=("uniform", "gaussian", "laplacian", "affine"),
             granularity: str = "block",
             extra_codebooks: dict | None = None,
             iters: int = 3) -> QuantResult:
    """Quantise tensor x to 2-bit with one scale per group and LUT selection.

    granularity: "block"  -> each block picks its own LUT (2-bit id/block)
                 "tensor" -> one LUT for the whole tensor (id stored once)
    extra_codebooks: {name: 4-point array}, e.g. a k-means codebook.
    """
    x = np.asarray(x)
    if x.size == 0:
        raise ValueError("input is empty")
    if x.ndim == 0:
        raise ValueError("input must contain at least one dimension")
    if not np.issubdtype(x.dtype, np.number):
        raise ValueError("input must be numeric")
    if not np.all(np.isfinite(x)):
        raise ValueError("input contains NaN or Inf")
    if granularity not in {"block", "tensor"}:
        raise ValueError("granularity must be 'block' or 'tensor'")
    w, valid = _row_blocks(x, block_size)
    n = x.size
    B = w.shape[0]

    # cands: indices, scale, offset, MSE, source/deployed codebooks, int8 LUT
    names, cands = [], []
    for name in luts:
        if name == "affine":
            float_code = np.arange(4.0)
            # Keep the arithmetic grid exact. Its smaller integer magnitude is
            # absorbed by the fitted scale and avoids unequal rounded steps.
            lut, normalisation = np.arange(4, dtype=np.int8), 1.0
            deployed_code = lut.astype(np.float64)
            idx, s, m, mse = fit_affine(w, iters=iters, valid=valid,
                                         code=deployed_code)
            cands.append((idx, s, m, mse, None, deployed_code,
                          (lut, normalisation)))
        else:
            if name in GENERAL_CODEBOOKS:
                code = GENERAL_CODEBOOKS[name]
            else:
                code = symmetric_codebook(SYMMETRIC_R[name])
            lut, normalisation = codebook_to_int8(code)
            deployed_code = lut.astype(np.float64)
            idx, s, mse = fit_scaled(w, deployed_code, iters=iters, valid=valid)
            cands.append((idx, s, np.zeros(B), mse, code, deployed_code,
                          (lut, normalisation)))
        names.append(name)
    for name, code in (extra_codebooks or {}).items():
        code = np.asarray(code, dtype=np.float64)
        if code.shape != (4,):
            raise ValueError(f"extra codebook {name!r} must contain four values")
        lut, normalisation = codebook_to_int8(code)
        deployed_code = lut.astype(np.float64)
        idx, s, mse = fit_scaled(w, deployed_code, iters=iters, valid=valid)
        cands.append((idx, s, np.zeros(B), mse, code, deployed_code,
                      (lut, normalisation)))
        names.append(name)

    mse_all = np.stack([c[3] for c in cands])            # (C, B)
    if granularity == "block" and len(cands) > 4:
        raise ValueError("two-bit per-block selectors support at most four LUTs")
    counts = valid.sum(axis=1)
    if granularity == "tensor":
        weighted = np.sum(mse_all * counts[None, :], axis=1) / counts.sum()
        pick = np.full(B, int(np.argmin(weighted)), dtype=np.uint8)
    else:
        pick = np.argmin(mse_all, axis=0).astype(np.uint8)

    indices = np.empty((B, block_size), dtype=np.uint8)
    scales = np.empty(B)
    mins = np.zeros(B)
    for c, (idx, s, m, _, _, _, _) in enumerate(cands):
        sel = pick == c
        indices[sel] = idx[sel]
        scales[sel] = s[sel]
        mins[sel] = m[sel]

    chosen_mse = mse_all[pick, np.arange(B)]
    codebooks, luts_i8 = {}, {}
    for name, (_, _, _, _, code, deployed_code, lut_data) in zip(names, cands):
        codebooks[name] = None if code is None else deployed_code
        luts_i8[name] = lut_data[0]

    total_mse = float(np.sum(chosen_mse * counts) / counts.sum())

    return QuantResult(
        shape=tuple(np.shape(x)), numel=n, block_size=block_size,
        granularity=granularity, lut_names=names,
        codebooks=codebooks, luts_int8=luts_i8,
        lut_ids=pick, indices=indices, scales=scales, mins=mins,
        mse=total_mse,
        per_lut_mse={nm: float(np.sum(mse_all[i] * counts) / counts.sum())
                     for i, nm in enumerate(names)},
        selection_hist={nm: float((pick == i).mean()) for i, nm in enumerate(names)},
        valid=valid, candidate_mse=mse_all,
    )


def dequantize(qr: QuantResult, use_int8_lut: bool = False) -> np.ndarray:
    """Reconstruct the tensor. use_int8_lut simulates the LUTI2 path.

    The stored weight scale converts the selected integer LUT entry directly
    to a reconstructed weight value.
    """
    B, N = qr.indices.shape
    out = np.empty((B, N))
    for i, name in enumerate(qr.lut_names):
        sel = qr.lut_ids == i
        if not sel.any():
            continue
        if use_int8_lut:
            lut = qr.luts_int8[name]
            vals = lut.astype(np.float64)[qr.indices[sel]]
            out[sel] = qr.scales[sel, None] * vals \
                + qr.mins[sel, None]
        else:
            code = qr.codebooks[name]
            if code is None:
                lut = qr.luts_int8[name]
                cb = lut.astype(np.float64)
            else:
                cb = code
            out[sel] = qr.scales[sel, None] * cb[qr.indices[sel]] \
                + qr.mins[sel, None]
    return out[qr.valid].reshape(qr.shape)


# ------------------------------------------------------------ pack / report

def pack_indices(idx: np.ndarray, order: str = "lsb-first") -> np.ndarray:
    """Pack 2-bit indices, 4 per byte, little-endian within the byte.
    Real kernel layouts interleave to match the LUTI2 expansion order --
    adjust here to mirror your packer."""
    flat = idx.astype(np.uint8).reshape(-1)
    if order not in {"lsb-first", "msb-first"}:
        raise ValueError("order must be 'lsb-first' or 'msb-first'")
    if np.any(flat > 3):
        raise ValueError("indices must be in the range 0..3")
    pad = (-flat.size) % 4
    if pad:
        flat = np.concatenate([flat, np.zeros(pad, np.uint8)])
    q = flat.reshape(-1, 4)
    shifts = (0, 2, 4, 6) if order == "lsb-first" else (6, 4, 2, 0)
    return np.bitwise_or.reduce(
        np.stack([q[:, i] << shifts[i] for i in range(4)]), axis=0
    ).astype(np.uint8)


def unpack_indices(packed: np.ndarray, count: int,
                   order: str = "lsb-first") -> np.ndarray:
    if order not in {"lsb-first", "msb-first"}:
        raise ValueError("order must be 'lsb-first' or 'msb-first'")
    p = packed.reshape(-1, 1)
    shifts = (0, 2, 4, 6) if order == "lsb-first" else (6, 4, 2, 0)
    out = np.concatenate([(p >> s) & 3 for s in shifts], axis=1)
    return out.reshape(-1)[:count].astype(np.uint8)


def snr_db(x: np.ndarray, x_hat: np.ndarray) -> float:
    err = np.mean((np.asarray(x, np.float64) - x_hat) ** 2)
    return float(10.0 * np.log10(max(np.var(x), EPS) / max(err, EPS)))


def bits_per_weight(qr: QuantResult, scale_bits: int = 32) -> float:
    bpw = 2.0 + scale_bits / qr.block_size
    if qr.granularity == "block":
        bpw += 2.0 / qr.block_size                       # LUT id
    if any(qr.lut_names[i] == "affine" and (qr.lut_ids == i).any()
           for i in range(len(qr.lut_names))):
        bpw += scale_bits / qr.block_size                # per-block min
    return bpw


def emit_zt0(qr: QuantResult) -> str:
    lines = []
    for name in qr.lut_names:
        lut = qr.luts_int8[name]
        lines.append(f"{name:>10s}: ZT0[0..3] = "
                     f"[{', '.join(f'{v:4d}' for v in lut)}]")
    return "\n".join(lines)


def report(qr: QuantResult, x=None) -> None:
    print(f"tensor {qr.shape}, block={qr.block_size}, "
          f"granularity={qr.granularity}, "
          f"~{bits_per_weight(qr):.3f} nominal bpw (assuming FP32 metadata)")
    print(f"selected-mix MSE: {qr.mse:.6g}")
    if x is not None:
        print(f"selected-mix SNR: {snr_db(x, dequantize(qr)):.2f} dB "
              f"(int8-LUT path: {snr_db(x, dequantize(qr, True)):.2f} dB)")
    print("single-LUT MSE  :", {k: round(v, 6) for k, v in qr.per_lut_mse.items()})
    print("block selection :", {k: f"{v:.1%}" for k, v in qr.selection_hist.items()})
    print(emit_zt0(qr))


def load_input(path: Path) -> np.ndarray:
    """Load a numeric .npy, .csv, or whitespace-delimited tensor."""
    if path.suffix.lower() == ".npy":
        data = np.load(path, allow_pickle=False)
    else:
        delimiter = "," if path.suffix.lower() == ".csv" else None
        data = np.loadtxt(path, delimiter=delimiter)
    data = np.asarray(data)
    if data.size == 0:
        raise ValueError("input is empty")
    if data.ndim == 0:
        raise ValueError("input must contain at least one dimension")
    if not np.issubdtype(data.dtype, np.number) or not np.all(np.isfinite(data)):
        raise ValueError("input must contain only finite numeric values")
    return data


def _db_gain(x: np.ndarray, better: QuantResult, reference: QuantResult) -> float:
    return snr_db(x, dequantize(better, True)) - snr_db(x, dequantize(reference, True))


def recommend(x: np.ndarray, block_size: int = 32, allow_affine: bool = False,
              include_block: bool = False, seed: int = 0
              ) -> tuple[QuantResult, QuantResult | None, dict]:
    """Recommend a shared LUT and optionally assess experimental block switching.

    The shared search includes fixed LUTs, optimal-r, and k-means. Block
    switching is deliberately limited to four fixed candidates so its selector
    remains exactly two bits per block.
    """
    blocks, valid = _row_blocks(np.asarray(x), block_size)
    r = best_symmetric_r(blocks, valid=valid)
    custom = {
        "opt-r": symmetric_codebook(r),
        "kmeans": kmeans_codebook(blocks, seed=seed, valid=valid),
    }
    tensor_luts = ["uniform", "gaussian", "laplacian", "int2-grid"]
    if allow_affine:
        tensor_luts.append("affine")
    tensor = quantize(x, block_size, tuple(tensor_luts), "tensor", custom)

    block = None
    if include_block:
        block_luts = ("uniform", "gaussian", "laplacian", "affine") \
            if allow_affine else ("uniform", "gaussian", "laplacian", "int2-grid")
        block = quantize(x, block_size, block_luts, "block")

    baseline = quantize(x, block_size, ("int2-grid",), "tensor")
    selected_name = tensor.lut_names[int(tensor.lut_ids[0])]
    tensor_snr = snr_db(x, dequantize(tensor, True))
    baseline_snr = snr_db(x, dequantize(baseline, True))
    payload = {
        "input": {"shape": list(np.shape(x)), "elements": int(np.size(x))},
        "configuration": {
            "block_size": block_size,
            "affine_allowed": allow_affine,
            "metric": "MSE through emitted int8 LUT",
        },
        "recommended_tensor_lut": selected_name,
        "optimal_symmetric_ratio": r,
        "tensor": _result_summary(tensor, x),
        "int2_grid_baseline_snr_db": baseline_snr,
        "tensor_gain_over_int2_grid_db": tensor_snr - baseline_snr,
    }
    if block is not None:
        gain = _db_gain(x, block, tensor)
        payload["experimental_block_switching"] = {
            **_result_summary(block, x),
            "gain_over_recommended_tensor_lut_db": gain,
            "selector_bits_per_weight": 2.0 / block_size,
            "interpretation": (
                "gain is at least 0.25 dB; compare it with measured kernel switching cost"
                if gain >= 0.25 else
                "gain is below 0.25 dB; kernel switching cost may dominate"
                if gain > 0 else
                "no accuracy gain over the recommended shared LUT"
            ),
        }
    return tensor, block, payload


def _result_summary(qr: QuantResult, x: np.ndarray) -> dict:
    selected = sorted({qr.lut_names[int(i)] for i in qr.lut_ids})
    return {
        "scope": qr.granularity,
        "selected_luts": selected,
        "mse": qr.mse,
        "snr_db": snr_db(x, dequantize(qr, True)),
        "bits_per_weight": bits_per_weight(qr),
        "candidate_mse": qr.per_lut_mse,
        "selection_histogram": qr.selection_hist,
        "int8_luts": {
            name: {"values": lut.tolist()}
            for name, lut in qr.luts_int8.items()
        },
    }


def write_outputs(qr: QuantResult, codes_out: Path | None = None,
                  packed_out: Path | None = None,
                  selectors_out: Path | None = None,
                  pack_order: str = "lsb-first") -> None:
    """Write codes in logical shape and deployment buffers with row padding."""
    if codes_out is not None:
        np.save(codes_out, qr.indices[qr.valid].reshape(qr.shape))
    if packed_out is not None:
        packed_out.write_bytes(pack_indices(qr.indices, pack_order).tobytes())
    if selectors_out is not None:
        if qr.granularity != "block":
            raise ValueError("selector output requires block LUT scope")
        selectors_out.write_bytes(pack_indices(qr.lut_ids, pack_order).tobytes())


def _c_array(name: str, c_type: str, values, formatter=str) -> str:
    values = list(values)
    lines = [f"static const {c_type} {name}[{len(values)}] = {{"]
    for start in range(0, len(values), 12):
        lines.append("    " + ", ".join(formatter(v) for v in
                                           values[start:start + 12]) + ",")
    lines.append("};")
    return "\n".join(lines)


def write_c_fixture(path: Path, qr: QuantResult,
                    pack_order: str = "lsb-first") -> None:
    """Emit a self-describing C fixture for a packer/kernel integration test."""
    packed = pack_indices(qr.indices, pack_order)
    if qr.granularity == "tensor":
        names = [qr.lut_names[int(qr.lut_ids[0])]]
        selector_ids = np.zeros_like(qr.lut_ids)
    else:
        names = qr.lut_names
        selector_ids = qr.lut_ids
    selectors = pack_indices(selector_ids, pack_order)
    luts = np.concatenate([qr.luts_int8[name] for name in names])
    def f32(value) -> str:
        literal = f"{float(value):.9g}"
        if "." not in literal and "e" not in literal.lower():
            literal += ".0"
        return literal + "f"
    text = "\n".join([
        "/* Generated by lut2quant. */",
        "#ifndef LUT2QUANT_GENERATED_FIXTURE_H",
        "#define LUT2QUANT_GENERATED_FIXTURE_H",
        "#include <stdint.h>",
        f"#define LUT2Q_ELEMENT_COUNT {qr.numel}",
        f"#define LUT2Q_BLOCK_SIZE {qr.block_size}",
        f"#define LUT2Q_BLOCK_COUNT {qr.indices.shape[0]}",
        f"#define LUT2Q_LUT_COUNT {len(names)}",
        f"#define LUT2Q_PACK_ORDER_LSB_FIRST {int(pack_order == 'lsb-first')}",
        _c_array("lut2q_luts", "int8_t", luts, lambda v: str(int(v))),
        _c_array("lut2q_block_scales", "float", qr.scales, f32),
        _c_array("lut2q_block_offsets", "float", qr.mins, f32),
        _c_array("lut2q_packed_codes", "uint8_t", packed,
                 lambda v: str(int(v))),
        _c_array("lut2q_packed_selectors", "uint8_t", selectors,
                 lambda v: str(int(v))),
        "#endif /* LUT2QUANT_GENERATED_FIXTURE_H */",
        "",
    ])
    path.write_text(text, encoding="utf-8")


# ---------------------------------------------------------------------- demo

def _demo_tensors(n: int, rng, block_size: int = 32) -> dict:
    t = {
        "gaussian": rng.standard_normal(n),
        "laplacian": rng.laplace(0.0, 1 / np.sqrt(2), n),
        "uniform": rng.uniform(-1.0, 1.0, n),
        "student_t3": rng.standard_t(3, n) / np.sqrt(3.0),
        "skewed": np.exp(0.5 * rng.standard_normal(n)) - 1.0,
    }
    # Alternate complete blocks from different distributions: the structured
    # case that per-block LUT selection is intended to capture.
    parts = [t["gaussian"], t["laplacian"], t["uniform"], t["skewed"]]
    block_count = n // block_size
    source_blocks = [p[: block_count * block_size].reshape(block_count,
                                                            block_size)
                     for p in parts]
    mixed_blocks = [source_blocks[i % len(parts)][i] for i in range(block_count)]
    mix = np.asarray(mixed_blocks).reshape(-1)
    if mix.size < n:
        mix = np.concatenate([mix, parts[block_count % len(parts)][: n - mix.size]])
    t["blockmix"] = mix
    return t


def demo(block_size: int = 32, n: int = 1 << 16, seed: int = 0,
         granularity: str = "block") -> None:
    rng = np.random.default_rng(seed)
    tensors = _demo_tensors(n, rng, block_size)
    fams = ["uniform", "gaussian", "laplacian", "affine"]

    cols = fams + ["sel-sym", "sel-all", "opt-r", "kmeans"]
    if granularity == "block":
        selection_help = f"""\
  sel-sym    each block of {block_size} picks the best of 3 symmetric tables
             (2-bit tag per block, {2 / block_size:.4f} bpw)
  sel-all    as sel-sym, with affine also on the menu (wins partly through
             affine's extra per-block parameter -- compare budgets, not columns)"""
        switching_help = ("Per-block selection also needs a kernel strategy for "
                          "switching tables (see tutorial Section 8.1).")
    else:
        selection_help = """\
  sel-sym    one symmetric table for the whole tensor, chosen from 3 candidates
  sel-all    one table for the whole tensor, with affine also considered"""
        switching_help = "Tensor selection stores no per-block selector."
    print(f"""
Each column is a strategy a user could adopt for choosing the 4-entry LUT:
  uniform/gaussian/laplacian  one pre-designed symmetric table for the whole
                              tensor (textbook-optimal IF the data has that shape)
  affine     pre-designed {{0,1,2,3}} x scale + offset table for lopsided data
             (note: stores an extra FP32 offset, +1 bpw at block size 32)
{selection_help}
  opt-r      one CUSTOM symmetric table per tensor: sweep the inner/outer
             ratio r and keep the best setting
  kmeans     one CUSTOM table per tensor: clustering places all 4 values
             (may come out asymmetric)
Shared custom tables require no per-block selector. {switching_help}
"int2-grid", the {{-2,-1,0,1}} arithmetic baseline, is included by the
recommender but omitted here to keep the teaching comparison compact.""")
    print(f"\nSNR in dB (higher is better), block={block_size}, "
          f"selection granularity={granularity}\n")
    print(f"{'tensor':>12s} | " + " | ".join(f"{c:>9s}" for c in cols))
    print("-" * (15 + 12 * len(cols)))

    for name, x in tensors.items():
        w, valid = _row_blocks(x, block_size)
        row = []
        for f in fams:
            qr = quantize(x, block_size, luts=(f,), granularity="tensor")
            row.append(snr_db(x, dequantize(qr)))
        qr_sym = quantize(x, block_size, luts=tuple(fams[:3]),
                          granularity=granularity)
        row.append(snr_db(x, dequantize(qr_sym)))
        qr_sel = quantize(x, block_size, luts=tuple(fams),
                          granularity=granularity)
        row.append(snr_db(x, dequantize(qr_sel)))
        r_opt = best_symmetric_r(w, valid=valid)
        qr_r = quantize(x, block_size, luts=(),
                        extra_codebooks={"opt-r": symmetric_codebook(r_opt)},
                        granularity="tensor")
        row.append(snr_db(x, dequantize(qr_r)))
        cb = kmeans_codebook(w, valid=valid)
        qr_k = quantize(x, block_size, luts=(),
                        extra_codebooks={"kmeans": cb}, granularity="tensor")
        row.append(snr_db(x, dequantize(qr_k)))
        print(f"{name:>12s} | " + " | ".join(f"{v:9.2f}" for v in row))
        extra = (f"    sel-all mix: "
                 + ", ".join(f"{k} {v:.0%}" for k, v in
                             qr_sel.selection_hist.items() if v > 0.005)
                 + f"   |   opt-r = {r_opt:.2f}"
                 + f"   |   kmeans = ["
                 + ", ".join(f"{c:+.3f}" for c in cb) + "]")
        print(extra)

    # round-trip check on the packed format
    x = tensors["gaussian"]
    qr = quantize(x, block_size)
    packed = pack_indices(qr.indices)
    assert np.array_equal(unpack_indices(packed, qr.indices.size),
                          qr.indices.reshape(-1))
    print("\npack/unpack round-trip: OK "
          f"({packed.size} bytes for {qr.indices.size} weights)")
    print(f"\nint8 LUTs for ZT0 ({', '.join(qr.lut_names)}):")
    print(emit_zt0(qr))
