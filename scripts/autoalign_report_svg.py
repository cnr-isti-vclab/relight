#!/usr/bin/env python3
"""Render an auto-alignment JSON report as a multi-column SVG overview."""

import argparse
import base64
import html
import io
import json
import math
import sys
from pathlib import Path

from PIL import Image


THUMBNAIL_SIZE = 64
CARD_WIDTH = 430
CARD_HEIGHT = 236
CARD_GAP = 12
OUTER_MARGIN = 20
HISTOGRAM_BINS = 10
HISTOGRAM_MAX = 10.0
MIN_IMAGE_SIDE = 1024


def escaped(value):
    return html.escape(str(value), quote=True)


def display_path(path, root, max_length=54):
    try:
        label = str(Path(path).relative_to(root))
    except ValueError:
        label = str(path)
    if len(label) > max_length:
        label = "\u2026" + label[-(max_length - 1) :]
    return label


def thumbnail_data(image_path):
    try:
        with Image.open(image_path) as image:
            image.load()
            image = image.convert("RGB")
            image.thumbnail(
                (THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                Image.Resampling.LANCZOS,
            )
            canvas = Image.new("RGB", (THUMBNAIL_SIZE, THUMBNAIL_SIZE), "#202a38")
            canvas.paste(
                image,
                (
                    (THUMBNAIL_SIZE - image.width) // 2,
                    (THUMBNAIL_SIZE - image.height) // 2,
                ),
            )
        buffer = io.BytesIO()
        canvas.save(buffer, format="JPEG", quality=82)
        return "data:image/jpeg;base64," + base64.b64encode(buffer.getvalue()).decode("ascii")
    except OSError as error:
        print(f"Could not load thumbnail {image_path}: {error}", file=sys.stderr)
        return None


def image_size(image_path):
    try:
        with Image.open(image_path) as image:
            return image.size
    except OSError as error:
        print(f"Could not inspect image dimensions {image_path}: {error}", file=sys.stderr)
        return None


def qualifies_by_size(image_path):
    dimensions = image_size(image_path)
    return dimensions is not None and (
        dimensions[0] > MIN_IMAGE_SIDE or dimensions[1] > MIN_IMAGE_SIDE
    )


def qualifying_rows(dataset, folder):
    rows = []
    for item in dataset["images"]:
        if qualifies_by_size(folder / item["image"]):
            rows.append(item)
    return rows


def first_qualifying_image(folder):
    try:
        candidates = sorted(
            path
            for path in folder.iterdir()
            if path.is_file()
            and not path.name.startswith("._")
            and path.suffix in (".jpg", ".JPG")
        )
    except OSError as error:
        print(f"Could not list images in {folder}: {error}", file=sys.stderr)
        return None
    return next((path for path in candidates if qualifies_by_size(path)), None)


def text_element(x, y, text, size=10, fill="#263445", weight=None):
    weight_attribute = f' font-weight="{weight}"' if weight else ""
    return (
        f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}"'
        f'{weight_attribute}>{escaped(text)}</text>'
    )


def render_histogram(x, y, width, title, values, color):
    count = len(values)
    overflow = sum(value > HISTOGRAM_MAX for value in values)
    counts = [0] * HISTOGRAM_BINS
    for value in values:
        index = min(
            HISTOGRAM_BINS - 1,
            int(max(0.0, value) / HISTOGRAM_MAX * HISTOGRAM_BINS),
        )
        counts[index] += 1

    caption = f"{title} | n={count} | >10px={overflow}"
    parts = [text_element(x, y, caption, size=10, weight="600")]
    plot_x = x + 75
    plot_y = y + 7
    plot_width = width - 88
    plot_height = 29

    for tick in range(5):
        tick_x = plot_x + plot_width * tick / 4
        parts.append(
            f'<line x1="{tick_x:.2f}" y1="{plot_y}" '
            f'x2="{tick_x:.2f}" y2="{plot_y + plot_height}" '
            'stroke="#dce3ec" stroke-width="1"/>'
        )

    peak = max(counts, default=0)
    bar_width = plot_width / HISTOGRAM_BINS
    for index, value in enumerate(counts):
        bar_height = plot_height * value / peak if peak else 0
        bar_x = plot_x + index * bar_width + 1
        bar_y = plot_y + plot_height - bar_height
        parts.append(
            f'<rect x="{bar_x:.2f}" y="{bar_y:.2f}" '
            f'width="{max(0, bar_width - 2):.2f}" height="{bar_height:.2f}" '
            f'rx="1" fill="{color}"/>'
        )
    parts.append(
        f'<line x1="{plot_x}" y1="{plot_y + plot_height}" '
        f'x2="{plot_x + plot_width}" y2="{plot_y + plot_height}" '
        'stroke="#68788d" stroke-width="1"/>'
    )
    parts.append(text_element(plot_x, plot_y + plot_height + 12, "0", size=8, fill="#59697d"))
    parts.append(
        f'<text x="{plot_x + plot_width}" y="{plot_y + plot_height + 12}" '
        'font-size="8" fill="#59697d" text-anchor="end">'
        "10px</text>"
    )
    return "".join(parts)


def render_success_card(dataset, rows, x, y, root):
    folder = Path(dataset["dataset"])
    path_label = display_path(dataset["dataset"], root)
    parts = [
        f'<g><title>{escaped(dataset["dataset"])}</title>',
        f'<rect x="{x}" y="{y}" width="{CARD_WIDTH}" height="{CARD_HEIGHT}" '
        'rx="7" fill="#ffffff" stroke="#cbd5e1"/>',
        text_element(x + 12, y + 20, path_label, size=12, weight="700"),
    ]
    thumbnail = thumbnail_data(folder / rows[0]["image"])
    if thumbnail:
        parts.append(
            f'<image x="{x + 12}" y="{y + 31}" width="{THUMBNAIL_SIZE}" '
            f'height="{THUMBNAIL_SIZE}" href="{thumbnail}" '
            'preserveAspectRatio="xMidYMid meet"/>'
        )
    else:
        parts.extend(
            [
                f'<rect x="{x + 12}" y="{y + 31}" width="{THUMBNAIL_SIZE}" '
                f'height="{THUMBNAIL_SIZE}" rx="3" fill="#e8edf3"/>',
                text_element(x + 18, y + 66, "No image", size=9, fill="#68788d"),
            ]
        )
    parts.extend(
        [
            text_element(
                x + 88,
                y + 44,
                f"Images >1024px: {len(rows)} / {dataset['image_count']}",
                size=10,
            ),
            text_element(
                x + 88,
                y + 59,
                f"First included: {rows[0]['image']}",
                size=9,
            ),
            text_element(
                x + 88,
                y + 74,
                f"Selected baseline mean/max: "
                f"{sum(math.hypot(item['baseline_offset_x'], item['baseline_offset_y']) for item in rows) / len(rows):.2f} / "
                f"{max(math.hypot(item['baseline_offset_x'], item['baseline_offset_y']) for item in rows):.2f}px",
                size=9,
            ),
            text_element(
                x + 88,
                y + 89,
                f"Selected error mean/max: "
                f"{sum(math.hypot(item['error_x'], item['error_y']) for item in rows) / len(rows):.2f} / "
                f"{max(math.hypot(item['error_x'], item['error_y']) for item in rows):.2f}px",
                size=9,
            ),
        ]
    )

    baseline_lengths = [
        math.hypot(item["baseline_offset_x"], item["baseline_offset_y"])
        for item in rows
    ]
    error_lengths = [
        math.hypot(item["error_x"], item["error_y"])
        for item in rows
    ]
    parts.append(
        render_histogram(
            x + 12,
            y + 111,
            CARD_WIDTH - 24,
            "Baseline offset length",
            baseline_lengths,
            "#4285d4",
        )
    )
    parts.append(
        render_histogram(
            x + 12,
            y + 170,
            CARD_WIDTH - 24,
            "|Expected - measured| length",
            error_lengths,
            "#e07a42",
        )
    )
    parts.append("</g>")
    return "".join(parts)


def render_failure_card(failure, thumbnail_path, x, y, root):
    folder = Path(failure["dataset"])
    path_label = display_path(failure["dataset"], root)
    parts = [
        f'<g><title>{escaped(failure["dataset"])}: {escaped(failure["error"])}</title>',
        f'<rect x="{x}" y="{y}" width="{CARD_WIDTH}" height="{CARD_HEIGHT}" '
        'rx="7" fill="#fffafa" stroke="#e2a6a6"/>',
        text_element(x + 12, y + 20, path_label, size=12, weight="700", fill="#842d2d"),
    ]
    thumbnail = thumbnail_data(thumbnail_path) if thumbnail_path else None
    if thumbnail:
        parts.append(
            f'<image x="{x + 12}" y="{y + 31}" width="{THUMBNAIL_SIZE}" '
            f'height="{THUMBNAIL_SIZE}" href="{thumbnail}" '
            'preserveAspectRatio="xMidYMid meet"/>'
        )
    else:
        parts.extend(
            [
                f'<rect x="{x + 12}" y="{y + 31}" width="{THUMBNAIL_SIZE}" '
                f'height="{THUMBNAIL_SIZE}" rx="3" fill="#f5e6e6"/>',
                text_element(x + 18, y + 66, "No image", size=9, fill="#842d2d"),
            ]
        )

    parts.append(text_element(x + 88, y + 43, "Not measured", size=11, fill="#842d2d", weight="700"))
    message = failure["error"]
    words = message.split()
    lines = []
    current = ""
    for word in words:
        candidate = f"{current} {word}".strip()
        if len(candidate) > 52 and current:
            lines.append(current)
            current = word
        else:
            current = candidate
    if current:
        lines.append(current)
    for index, line in enumerate(lines[:7]):
        parts.append(
            text_element(
                x + 88,
                y + 59 + index * 13,
                line,
                size=8,
                fill="#59697d",
            )
        )
    if len(lines) > 7:
        parts.append(text_element(x + 88, y + 150, "…", size=10, fill="#59697d"))
    parts.append("</g>")
    return "".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--report",
        type=Path,
        default=Path.home() / "tmp/autoalign_report.json",
        help="auto-alignment JSON report",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path.home() / "tmp/autoalign_report.svg",
        help="destination SVG file",
    )
    parser.add_argument(
        "--columns",
        type=int,
        default=4,
        help="number of dataset cards per row (default: %(default)s)",
    )
    args = parser.parse_args()
    if args.columns < 1 or args.columns > 8:
        parser.error("--columns must be between 1 and 8")
    if not args.report.is_file():
        parser.error(f"report does not exist: {args.report}")

    report = json.loads(args.report.read_text(encoding="utf-8"))
    root = Path(report["root"])
    successes = sorted(report["datasets"], key=lambda item: item["dataset"])
    failures = sorted(report.get("failures", []), key=lambda item: item["dataset"])
    success_cards = []
    baseline_lengths = []
    error_lengths = []
    for dataset in successes:
        rows = qualifying_rows(dataset, Path(dataset["dataset"]))
        if rows:
            success_cards.append((dataset, rows))
            baseline_lengths.extend(
                math.hypot(item["baseline_offset_x"], item["baseline_offset_y"])
                for item in rows
            )
            error_lengths.extend(
                math.hypot(item["error_x"], item["error_y"])
                for item in rows
            )

    failure_cards = []
    for failure in failures:
        thumbnail_path = first_qualifying_image(Path(failure["dataset"]))
        if thumbnail_path:
            failure_cards.append((failure, thumbnail_path))

    cards = [(True, item) for item in success_cards] + [
        (False, item) for item in failure_cards
    ]
    rows = math.ceil(len(cards) / args.columns)
    width = OUTER_MARGIN * 2 + args.columns * CARD_WIDTH + (args.columns - 1) * CARD_GAP
    content_top = 126
    height = content_top + rows * CARD_HEIGHT + max(0, rows - 1) * CARD_GAP + OUTER_MARGIN
    omitted_datasets = len(successes) - len(success_cards) + len(failures) - len(failure_cards)
    baseline_mean = sum(baseline_lengths) / len(baseline_lengths) if baseline_lengths else 0
    baseline_max = max(baseline_lengths, default=0)
    error_mean = sum(error_lengths) / len(error_lengths) if error_lengths else 0
    error_max = max(error_lengths, default=0)

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        "<style>text{font-family:Arial,Helvetica,sans-serif}</style>",
        f'<rect width="{width}" height="{height}" fill="#f3f6fa"/>',
        text_element(
            OUTER_MARGIN,
            31,
            "Relight auto-alignment recap",
            size=22,
            fill="#172638",
            weight="700",
        ),
        text_element(
            OUTER_MARGIN,
            53,
            f"{len(success_cards)} measured datasets | "
            f"{len(failure_cards)} failed datasets with qualifying images | "
            f"{len(baseline_lengths)} qualifying images | "
            f"{omitted_datasets} dataset cards omitted (no qualifying image)",
            size=12,
            fill="#43546a",
        ),
        text_element(
            OUTER_MARGIN,
            75,
            f"Baseline vector length on included images: mean {baseline_mean:.2f}px, "
            f"max {baseline_max:.2f}px",
            size=11,
            fill="#315e91",
        ),
        text_element(
            OUTER_MARGIN,
            94,
            f"Expected-measured difference length on included images: mean "
            f"{error_mean:.2f}px, max {error_max:.2f}px",
            size=11,
            fill="#a95029",
        ),
        text_element(
            OUTER_MARGIN,
            112,
            "Images included only if width >1024 or height >1024 px. Histograms span 0-10px; values >10px use the last bin.",
            size=9,
            fill="#59697d",
        ),
    ]

    for index, (measured, item) in enumerate(cards):
        column = index % args.columns
        row = index // args.columns
        x = OUTER_MARGIN + column * (CARD_WIDTH + CARD_GAP)
        y = content_top + row * (CARD_HEIGHT + CARD_GAP)
        if measured:
            dataset, selected_rows = item
            svg.append(render_success_card(dataset, selected_rows, x, y, root))
        else:
            failure, thumbnail_path = item
            svg.append(render_failure_card(failure, thumbnail_path, x, y, root))

    svg.append("</svg>\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(svg), encoding="utf-8")
    print(
        f"Wrote {len(success_cards)} measured and {len(failure_cards)} failed "
        f"dataset cards to {args.output}"
    )


if __name__ == "__main__":
    main()
