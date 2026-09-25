# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

"""Command-line interface for :mod:`lut2quant`."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from .core import demo, load_input, recommend, write_c_fixture, write_outputs


def _print_recommendation_summary(payload: dict, report_path: Path) -> None:
    """Print the decision-oriented subset of a recommendation report."""
    name = payload["recommended_tensor_lut"]
    tensor = payload["tensor"]
    lut = tensor["int8_luts"][name]
    print(f"Recommended LUT: {name}")
    print(f"LUTI2 table (int8): {lut['values']}")
    print(f"Scale-group size: {payload['configuration']['block_size']}")
    print(f"Nominal storage: {tensor['bits_per_weight']:.4g} bpw")
    print()
    print("Reconstruction:")
    print(f"  Recommended LUT: {tensor['snr_db']:.2f} dB")
    print(f"  int2-grid:       {payload['int2_grid_baseline_snr_db']:.2f} dB")
    print(f"  Improvement:     {payload['tensor_gain_over_int2_grid_db']:+.2f} dB")
    print()
    print("Next step: validate model quality, then test this table with the target "
          "LUTI2 kernel.")
    print(f"Detailed results written to {report_path}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Recommend and export a 2-bit LUT quantizer"
    )
    parser.add_argument("input", nargs="?", type=Path,
                        help="input .npy, .csv, or whitespace-delimited tensor")
    parser.add_argument("--demo", action="store_true")
    parser.add_argument("--block", type=int, default=32)
    parser.add_argument("--n", type=int, default=1 << 16)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--granularity", choices=("block", "tensor"),
                        default="block", help="selection scope used by --demo")
    parser.add_argument("--lut-scope", choices=("tensor", "block"),
                        default="tensor",
                        help="selected result to export (block is experimental)")
    parser.add_argument("--compare-lut-scopes", action="store_true",
                        help="include experimental block switching in the report")
    parser.add_argument("--allow-affine", action="store_true",
                        help="include higher-cost scale-plus-offset candidates")
    parser.add_argument("--out", type=Path,
                        help="write recommendation report as JSON")
    parser.add_argument("--codes-out", type=Path,
                        help="write logical uint8 indices as .npy")
    parser.add_argument("--packed-codes-out", type=Path,
                        help="write packed blocks including row-tail padding")
    parser.add_argument("--selectors-out", type=Path,
                        help="write packed LUT selectors (block scope only)")
    parser.add_argument("--c-fixture-out", type=Path,
                        help="write a self-describing C integration fixture")
    parser.add_argument("--pack-order", choices=("lsb-first", "msb-first"),
                        default="lsb-first")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.demo:
        demo(args.block, args.n, args.seed, args.granularity)
        return 0
    if args.input is None:
        parser.error("input is required unless --demo is used")
    if args.selectors_out is not None and args.lut_scope != "block":
        parser.error("--selectors-out requires --lut-scope block")
    try:
        x = load_input(args.input)
        tensor, block, payload = recommend(
            x,
            args.block,
            allow_affine=args.allow_affine,
            include_block=args.compare_lut_scopes or args.lut_scope == "block",
            seed=args.seed,
        )
        chosen = block if args.lut_scope == "block" else tensor
        assert chosen is not None
        payload["exported_scope"] = args.lut_scope
        payload["packing"] = {
            "order": args.pack_order,
            "codes_include_row_tail_padding": True,
        }
        write_outputs(chosen, args.codes_out, args.packed_codes_out,
                      args.selectors_out, args.pack_order)
        if args.c_fixture_out is not None:
            write_c_fixture(args.c_fixture_out, chosen, args.pack_order)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))

    report = json.dumps(payload, indent=2) + "\n"
    if args.out is not None:
        args.out.write_text(report, encoding="utf-8")
        _print_recommendation_summary(payload, args.out)
    else:
        print(report, end="")
    return 0
