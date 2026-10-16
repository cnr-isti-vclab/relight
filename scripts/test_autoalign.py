#!/usr/bin/env python3
"""Measure relight-cli auto-alignment accuracy on .lp datasets.

Requires Pillow. By default this scans ~/Sync/project/RTI, uses ../bin/relight-cli,
and writes ~/tmp/autoalign_report.json. Each perturbed dataset is created in a
temporary directory under ~/tmp and removed after it has been measured.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import math
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


OFFSET_LINE = re.compile(
    r"^(.*) offset: "
    r"([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?) "
    r"([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)$"
)
MAX_TRANSLATION = 10


def image_files(folder):
    return sorted(
        path
        for path in folder.iterdir()
        if path.is_file()
        and not path.name.startswith("._")
        and path.suffix in (".jpg", ".JPG")
    )


def lp_files(folder):
    return sorted(
        path
        for path in folder.glob("*.lp")
        if not path.name.startswith("._")
    )


def run_alignment(executable, folder):
    result = subprocess.run(
        [str(executable), "-a", str(folder)],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"relight-cli exited with {result.returncode}: "
            f"{result.stderr.strip() or result.stdout.strip()}"
        )

    offsets = {}
    for line in result.stdout.splitlines():
        match = OFFSET_LINE.match(line.strip())
        if match:
            name, x, y = match.groups()
            offsets[name] = (float(x), float(y))
    if not offsets:
        raise RuntimeError(
            "No alignment offsets were found in relight-cli output. "
            "Check that this executable prints offsets after aligning."
        )
    return offsets


def offset_statistics(offsets):
    magnitudes = [math.hypot(x, y) for x, y in offsets.values()]
    return {
        "mean_x": sum(x for x, _ in offsets.values()) / len(offsets),
        "mean_y": sum(y for _, y in offsets.values()) / len(offsets),
        "mean_magnitude": sum(magnitudes) / len(magnitudes),
        "max_magnitude": max(magnitudes),
    }


def write_report(report_path, report):
    images = [image for dataset in report["datasets"] for image in dataset["images"]]
    baseline = {
        str(index): (image["baseline_offset_x"], image["baseline_offset_y"])
        for index, image in enumerate(images)
    }
    errors = [image["error_magnitude"] for image in images]
    report["summary"] = {
        "datasets_processed": len(report["datasets"]),
        "datasets_failed": len(report["failures"]),
        "images_processed": len(images),
        "baseline_offsets": offset_statistics(baseline) if baseline else None,
        "consistency": (
            {
                "mean_error_magnitude": sum(errors) / len(errors),
                "max_error_magnitude": max(errors),
            }
            if errors
            else None
        ),
    }
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=report_path.parent,
        prefix=report_path.name + ".",
        suffix=".tmp",
        delete=False,
    ) as temporary_file:
        json.dump(report, temporary_file, indent=2, ensure_ascii=False)
        temporary_file.write("\n")
        temporary_path = Path(temporary_file.name)
    try:
        temporary_path.replace(report_path)
    finally:
        temporary_path.unlink(missing_ok=True)


def make_translated_copy(source, destination, dx, dy):
    with Image.open(source) as image:
        image.load()
        width, height = image.size
        if width <= 2 * MAX_TRANSLATION or height <= 2 * MAX_TRANSLATION:
            raise ValueError(
                f"{source.name} is too small to crop {MAX_TRANSLATION} pixels "
                "from every edge"
            )

        margin = MAX_TRANSLATION
        canvas = Image.new(
            image.mode,
            (width + 2 * margin, height + 2 * margin),
            0,
        )
        canvas.paste(image, (margin + dx, margin + dy))
        cropped = canvas.crop(
            (
                2 * margin,
                2 * margin,
                width,
                height,
            )
        )
        cropped.save(destination, format="JPEG", quality=95)


def process_dataset(folder, executable, temporary_root, rng):
    dataset_lps = lp_files(folder)
    if not dataset_lps:
        raise ValueError("No .lp file found")
    lp_file = dataset_lps[0]
    try:
        count = int(lp_file.read_text(encoding="utf-8-sig").splitlines()[0].strip())
    except (IndexError, ValueError) as error:
        raise ValueError(f"Invalid first line in {lp_file.name}: {error}") from error

    images = image_files(folder)
    if count != len(images):
        raise ValueError(
            f"{lp_file.name} declares {count} images, found {len(images)} "
            "immediate .jpg/.JPG files"
        )
    if not images:
        raise ValueError("No immediate .jpg or .JPG images found")

    baseline = run_alignment(executable, folder)
    image_names = {path.name for path in images}
    if set(baseline) != image_names:
        raise RuntimeError(
            "Alignment output filenames do not match the dataset images "
            f"(missing: {sorted(image_names - set(baseline))}; "
            f"unexpected: {sorted(set(baseline) - image_names)})"
        )

    translations = {
        path.name: (
            rng.randint(-MAX_TRANSLATION, MAX_TRANSLATION),
            rng.randint(-MAX_TRANSLATION, MAX_TRANSLATION),
        )
        for path in images
    }

    with tempfile.TemporaryDirectory(
        prefix="relight-autoalign-", dir=temporary_root
    ) as temporary_dir:
        translated_folder = Path(temporary_dir)
        shutil.copy2(lp_file, translated_folder / lp_file.name)
        for path in images:
            dx, dy = translations[path.name]
            make_translated_copy(path, translated_folder / path.name, dx, dy)

        perturbed = run_alignment(executable, translated_folder)

    if set(perturbed) != image_names:
        raise RuntimeError(
            "Perturbed alignment output filenames do not match the dataset "
            f"(missing: {sorted(image_names - set(perturbed))}; "
            f"unexpected: {sorted(set(perturbed) - image_names)})"
        )

    reference = next(iter(baseline))
    reference_dx, reference_dy = translations[reference]
    per_image = []
    errors = []
    for name, (base_x, base_y) in baseline.items():
        aligned_x, aligned_y = perturbed[name]
        measured_x = aligned_x - base_x
        measured_y = aligned_y - base_y
        expected_x, expected_y = translations[name]
        expected_x -= reference_dx
        expected_y -= reference_dy
        error_x = measured_x - expected_x
        error_y = measured_y - expected_y
        errors.append(math.hypot(error_x, error_y))
        per_image.append(
            {
                "image": name,
                "translation_x": translations[name][0],
                "translation_y": translations[name][1],
                "baseline_offset_x": base_x,
                "baseline_offset_y": base_y,
                "perturbed_offset_x": aligned_x,
                "perturbed_offset_y": aligned_y,
                "measured_change_x": measured_x,
                "measured_change_y": measured_y,
                "expected_change_x": expected_x,
                "expected_change_y": expected_y,
                "error_x": error_x,
                "error_y": error_y,
                "error_magnitude": errors[-1],
            }
        )

    return {
        "dataset": str(folder),
        "lp_file": lp_file.name,
        "image_count": count,
        "reference_image": reference,
        "baseline_offsets": offset_statistics(baseline),
        "consistency": {
            "mean_error_magnitude": sum(errors) / len(errors),
            "max_error_magnitude": max(errors),
        },
        "images": per_image,
    }


def main():
    repository = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.home() / "Sync/project/RTI",
        help="root folder to search recursively",
    )
    parser.add_argument(
        "--relight-cli",
        type=Path,
        default=repository / "bin/relight-cli",
        help="path to relight-cli",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=Path.home() / "tmp/autoalign_report.json",
        help="JSON report destination",
    )
    parser.add_argument(
        "--seed",
        type=int,
        help="seed for repeatable random translations",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=min(8, os.cpu_count() or 1),
        help="datasets to process concurrently (default: %(default)s)",
    )
    parser.add_argument(
        "--resume",
        action="store_true",
        help="continue from completed datasets in the existing report",
    )
    args = parser.parse_args()

    root = args.root.expanduser().resolve()
    executable = args.relight_cli.expanduser().resolve()
    report_path = args.report.expanduser().resolve()
    temporary_root = Path.home() / "tmp"

    if not root.is_dir():
        parser.error(f"dataset root is not a directory: {root}")
    if not executable.is_file() or not executable.stat().st_mode & 0o111:
        parser.error(f"relight-cli is missing or not executable: {executable}")
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    temporary_root.mkdir(parents=True, exist_ok=True)
    report_path.parent.mkdir(parents=True, exist_ok=True)

    folders = [root, *(folder for folder in root.rglob("*") if folder.is_dir())]
    datasets = sorted(folder for folder in folders if lp_files(folder))
    if args.resume:
        if not report_path.is_file():
            parser.error(f"cannot resume; report does not exist: {report_path}")
        try:
            report = json.loads(report_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            parser.error(f"cannot read report for resuming: {error}")
        if report.get("root") != str(root) or report.get("relight_cli") != str(executable):
            parser.error("existing report belongs to a different dataset root or relight-cli")
        seed = report.get("seed")
        if not isinstance(seed, int) or (args.seed is not None and args.seed != seed):
            parser.error("resume seed does not match the existing report")
        if not isinstance(report.get("datasets"), list) or not isinstance(
            report.get("failures"), list
        ):
            parser.error("existing report has an invalid structure")
    else:
        seed = args.seed if args.seed is not None else random.SystemRandom().getrandbits(64)
        report = {
            "root": str(root),
            "relight_cli": str(executable),
            "seed": seed,
            "datasets": [],
            "failures": [],
        }

    seeds = random.Random(seed)
    dataset_seeds = {
        str(folder): seeds.getrandbits(64) for folder in datasets
    }
    completed = {item["dataset"] for item in report["datasets"]}
    pending = [folder for folder in datasets if str(folder) not in completed]
    pending_names = {str(folder) for folder in pending}
    report["failures"] = [
        item for item in report["failures"] if item["dataset"] not in pending_names
    ]

    write_report(report_path, report)
    print(
        f"Processing {len(pending)} datasets with {args.jobs} workers",
        flush=True,
    )
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(
                process_dataset,
                folder,
                executable,
                temporary_root,
                random.Random(dataset_seeds[str(folder)]),
            ): folder
            for folder in pending
        }
        for future in as_completed(futures):
            folder = futures[future]
            try:
                result = future.result()
                report["datasets"].append(result)
                baseline = result["baseline_offsets"]
                consistency = result["consistency"]
                print(
                    f"{folder}: baseline mean/max magnitude "
                    f"{baseline['mean_magnitude']:.3f}/"
                    f"{baseline['max_magnitude']:.3f}px; "
                    f"perturbation error mean/max "
                    f"{consistency['mean_error_magnitude']:.3f}/"
                    f"{consistency['max_error_magnitude']:.3f}px",
                    flush=True,
                )
            except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
                report["failures"].append(
                    {"dataset": str(folder), "error": str(error)}
                )
                print(f"{folder}: FAILED: {error}", file=sys.stderr, flush=True)
            report["datasets"].sort(key=lambda item: item["dataset"])
            report["failures"].sort(key=lambda item: item["dataset"])
            write_report(report_path, report)

    print(f"Report written to {report_path}")
    return 1 if report["failures"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
