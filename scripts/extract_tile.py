#!/usr/bin/env python3
"""
extract_tile.py — pick a 256×256 tile from an RTI dataset and export it.

Usage:
    python extract_tile.py [folder]

Click anywhere on the preview image to place the 256×256 tile (the click is
the tile centre). Press Enter or click "Export" to write the cropped images
and the .lp file to  <folder>_tile_<x>_<y>/.
"""

import os
import sys
import shutil
import glob
import argparse
import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk

TILE = 256

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_lp(folder):
    lps = glob.glob(os.path.join(folder, "*.lp"))
    return lps[0] if lps else None


def lp_count(lp_path):
    """Return the image count declared on the first line of the .lp file."""
    with open(lp_path) as f:
        for line in f:
            line = line.strip()
            if line:
                return int(line.split()[0])
    return 0


def image_files(folder, lp_path):
    """Return sorted JPEG paths in folder; warn if count differs from .lp header."""
    exts = (".jpg", ".jpeg", ".JPG", ".JPEG")
    jpegs = sorted(
        f for f in (
            os.path.join(folder, name) for name in os.listdir(folder)
        )
        if os.path.splitext(f)[1] in exts
    )
    expected = lp_count(lp_path)
    if len(jpegs) != expected:
        messagebox.showwarning(
            "Count mismatch",
            f".lp declares {expected} images but {len(jpegs)} JPEG(s) found in folder.\n"
            "Proceeding with the files found on disk."
        )
    return jpegs


def clamp_origin(cx, cy, img_w, img_h):
    """Return top-left (x0, y0) of a TILE×TILE box centred at (cx,cy)."""
    x0 = max(0, min(cx - TILE // 2, img_w - TILE))
    y0 = max(0, min(cy - TILE // 2, img_h - TILE))
    return x0, y0


# ---------------------------------------------------------------------------
# GUI
# ---------------------------------------------------------------------------

class App:
    def __init__(self, root, folder):
        self.root = root
        self.folder = folder
        self.lp_path = find_lp(folder)
        if not self.lp_path:
            messagebox.showerror("Error", "No .lp file found in folder.")
            root.destroy()
            return

        self.images = image_files(folder, self.lp_path)
        if not self.images:
            messagebox.showerror("Error", "No images listed in the .lp file were found.")
            root.destroy()
            return

        self.src_img = Image.open(self.images[0]).convert("RGB")
        self.img_w, self.img_h = self.src_img.size
        self.x0 = clamp_origin(self.img_w // 2, self.img_h // 2,
                                self.img_w, self.img_h)[0]
        self.y0 = clamp_origin(self.img_w // 2, self.img_h // 2,
                                self.img_w, self.img_h)[1]

        root.title("Extract 256×256 tile — click to place")
        self._build_ui()
        self._refresh()

    def _build_ui(self):
        # Scale image to fit inside 900×700 while keeping aspect ratio
        max_w, max_h = 900, 700
        scale = min(max_w / self.img_w, max_h / self.img_h, 1.0)
        self.scale = scale
        self.disp_w = int(self.img_w * scale)
        self.disp_h = int(self.img_h * scale)

        self.canvas = tk.Canvas(self.root, width=self.disp_w, height=self.disp_h,
                                cursor="crosshair")
        self.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.canvas.bind("<Button-1>", self._on_click)
        self.canvas.bind("<B1-Motion>", self._on_click)

        bar = tk.Frame(self.root)
        bar.pack(side=tk.BOTTOM, fill=tk.X, padx=6, pady=4)

        self.info_var = tk.StringVar()
        tk.Label(bar, textvariable=self.info_var, anchor="w").pack(side=tk.LEFT, expand=True)

        tk.Button(bar, text="Export", command=self._export,
                  bg="#2a7ae2", fg="white", padx=12).pack(side=tk.RIGHT)
        tk.Button(bar, text="Cancel", command=self.root.destroy,
                  padx=8).pack(side=tk.RIGHT, padx=4)

        self.root.bind("<Return>", lambda _: self._export())
        self.root.bind("<KP_Enter>", lambda _: self._export())

    def _on_click(self, event):
        cx = int(event.x / self.scale)
        cy = int(event.y / self.scale)
        self.x0, self.y0 = clamp_origin(cx, cy, self.img_w, self.img_h)
        self._refresh()

    def _refresh(self):
        disp = self.src_img.resize((self.disp_w, self.disp_h), Image.LANCZOS)
        self._tk_img = ImageTk.PhotoImage(disp)
        self.canvas.create_image(0, 0, anchor="nw", image=self._tk_img)

        # Draw selection rectangle
        rx0 = int(self.x0 * self.scale)
        ry0 = int(self.y0 * self.scale)
        rx1 = int((self.x0 + TILE) * self.scale)
        ry1 = int((self.y0 + TILE) * self.scale)
        self.canvas.create_rectangle(rx0, ry0, rx1, ry1,
                                     outline="#ff3030", width=2)
        # Semi-transparent overlay is hard in plain tkinter; darken border instead
        self.canvas.create_rectangle(rx0 + 1, ry0 + 1, rx1 - 1, ry1 - 1,
                                     outline="#ffffff", width=1, dash=(4, 4))

        self.info_var.set(
            f"Tile origin: ({self.x0}, {self.y0})   "
            f"→  ({self.x0 + TILE}, {self.y0 + TILE})   |   "
            f"{len(self.images)} images"
        )

    def _export(self):
        base = os.path.basename(self.folder.rstrip("/\\"))
        out_dir = os.path.join(
            os.path.dirname(self.folder),
            f"{base}_tile_{self.x0}_{self.y0}"
        )
        if os.path.exists(out_dir):
            if not messagebox.askyesno("Overwrite?",
                                       f"{out_dir}\nalready exists. Overwrite?"):
                return
        os.makedirs(out_dir, exist_ok=True)

        box = (self.x0, self.y0, self.x0 + TILE, self.y0 + TILE)

        total = len(self.images)
        prog_win = tk.Toplevel(self.root)
        prog_win.title("Exporting…")
        prog_win.resizable(False, False)
        prog_label = tk.Label(prog_win, text="", width=50)
        prog_label.pack(padx=12, pady=8)
        prog_bar_var = tk.DoubleVar()
        prog_canvas = tk.Canvas(prog_win, width=360, height=18)
        prog_canvas.pack(padx=12, pady=(0, 10))
        prog_win.update()

        for i, src_path in enumerate(self.images):
            name = os.path.basename(src_path)
            prog_label.config(text=f"[{i+1}/{total}]  {name}")
            prog_canvas.delete("all")
            prog_canvas.create_rectangle(0, 0, int(360 * (i + 1) / total), 18,
                                         fill="#2a7ae2", outline="")
            prog_win.update()

            img = Image.open(src_path)
            tile = img.crop(box)
            # Preserve original format/extension
            tile.save(os.path.join(out_dir, name))

        # Copy .lp unchanged (pixel coords in .lp are light directions, not image coords)
        shutil.copy2(self.lp_path, out_dir)

        prog_win.destroy()
        messagebox.showinfo("Done",
                            f"Exported {total} tiles + .lp to:\n{out_dir}")
        self.root.destroy()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Extract a 256×256 tile from an RTI dataset.")
    parser.add_argument("folder", nargs="?", help="Folder containing images and .lp")
    args = parser.parse_args()

    folder = args.folder
    if not folder:
        tmp = tk.Tk()
        tmp.withdraw()
        folder = filedialog.askdirectory(title="Select RTI dataset folder")
        tmp.destroy()
        if not folder:
            sys.exit(0)

    root = tk.Tk()
    app = App(root, folder)
    root.mainloop()
