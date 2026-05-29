#!/usr/bin/env python3
"""
Heightmap False-Color Filter
Highlights local height differences using a Gaussian-weighted neighborhood comparison.

Usage:
    python heightmap_filter.py <input.tiff> <gaussian_radius> [output.jpg]

Arguments:
    input.tiff        Input TIFF heightmap (normalized 0–1)
    gaussian_radius   Gaussian blur radius in pixels (e.g. 5, 10, 20)
    output.jpg        Output JPEG path (optional, defaults to <input>_diff.jpg)
"""

import sys
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter
import argparse
import os


# --- False-color palette ---
# Maps the signed difference [-1, 1] → RGB color
# Negative  = pixel is LOWER than neighborhood  → cool blues/purples
# Near zero = close to neighborhood average     → neutral green/white
# Positive  = pixel is HIGHER than neighborhood → warm oranges/reds

COLORMAP = np.array([
    # value   R     G     B
    [-1.00, 0.10, 0.05, 0.40],   # deep purple  (very low)
    [-0.60, 0.05, 0.20, 0.80],   # strong blue
    [-0.25, 0.20, 0.65, 0.90],   # cyan-blue
    [-0.05, 0.70, 0.90, 0.85],   # pale cyan     (slightly low)
    [ 0.00, 1.00, 1.00, 1.00],   # white         (flat)
    [ 0.05, 0.95, 0.95, 0.50],   # pale yellow   (slightly high)
    [ 0.25, 0.95, 0.75, 0.05],   # orange-yellow
    [ 0.60, 0.90, 0.25, 0.02],   # orange-red
    [ 1.00, 0.60, 0.02, 0.05],   # deep red      (very high)
], dtype=np.float32)


def apply_colormap(values: np.ndarray) -> np.ndarray:
    """Map a 2-D array of floats in [-1, 1] to RGB uint8 using the palette above."""
    stops = COLORMAP[:, 0]
    colors = COLORMAP[:, 1:]

    flat = values.ravel()
    r = np.interp(flat, stops, colors[:, 0])
    g = np.interp(flat, stops, colors[:, 1])
    b = np.interp(flat, stops, colors[:, 2])

    rgb = np.stack([r, g, b], axis=-1)
    rgb = np.clip(rgb * 255, 0, 255).astype(np.uint8)
    return rgb.reshape(values.shape[0], values.shape[1], 3)


def process(input_path: str, radius: float, output_path: str) -> None:
    # ── Load ────────────────────────────────────────────────────────────────
    img = Image.open(input_path)
    arr = np.array(img, dtype=np.float32)

    # Handle multi-band TIFFs (take first band / luminance)
    if arr.ndim == 3:
        if arr.shape[2] == 1:
            arr = arr[:, :, 0]
        else:
            # Weighted luminance if RGB; otherwise just first channel
            if arr.shape[2] >= 3:
                arr = 0.299 * arr[:, :, 0] + 0.587 * arr[:, :, 1] + 0.114 * arr[:, :, 2]
            else:
                arr = arr[:, :, 0]

    # Normalise to [0, 1] in case source isn't perfectly normalised
    lo, hi = arr.min(), arr.max()
    if hi > lo:
        arr = (arr - lo) / (hi - lo)

    # ── Gaussian blur  ───────────────────────────────────────────────────────
    # sigma ≈ radius / 2  (radius is the approximate 2-sigma extent)
    sigma = max(radius / 2.0, 0.5)
    blurred = gaussian_filter(arr, sigma=sigma)

    # ── Difference  ──────────────────────────────────────────────────────────
    diff = arr - blurred          # range roughly [-0.5, 0.5] in practice

    # Stretch contrast: find a robust percentile range
    p_low  = np.percentile(diff, 2)
    p_high = np.percentile(diff, 98)
    scale  = max(abs(p_low), abs(p_high), 1e-6)
    diff_norm = np.clip(diff / scale, -1.0, 1.0)

    # ── Colorize & save  ─────────────────────────────────────────────────────
    rgb = apply_colormap(diff_norm)
    out_img = Image.fromarray(rgb, mode="RGB")
    out_img.save(output_path, quality=95)

    print(f"✓  Input       : {input_path}  ({arr.shape[1]}×{arr.shape[0]} px)")
    print(f"   Gaussian σ  : {sigma:.2f} px  (radius={radius})")
    print(f"   Diff range  : [{diff.min():.4f}, {diff.max():.4f}]")
    print(f"   Stretch ±   : {scale:.4f}")
    print(f"✓  Output      : {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Apply false-color Gaussian-difference filter to a TIFF heightmap."
    )
    parser.add_argument("input",  help="Input TIFF heightmap (0–1 normalised)")
    parser.add_argument("radius", type=float,
                        help="Gaussian neighbourhood radius in pixels")
    parser.add_argument("output", nargs="?", default=None,
                        help="Output JPEG path (default: <input>_diff.jpg)")
    args = parser.parse_args()

    if args.output is None:
        base, _ = os.path.splitext(args.input)
        args.output = f"{base}_diff.jpg"

    process(args.input, args.radius, args.output)


if __name__ == "__main__":
    main()
