#!/usr/bin/env python3
"""
find_lp.py — Search the filesystem for folders containing .lp files,
create 256×256 thumbnails from the first image in each folder, and
produce a self-contained HTML index page.

Usage:
    python3 find_lp.py [search_root] [output_dir]

    search_root  directory to scan  (default: /)
    output_dir   where to write index.html + thumbs/  (default: ./lp_index)
"""

import os
import re
import sys
from datetime import datetime
from pathlib import Path

try:
    from PIL import Image
    HAS_PILLOW = True
except ImportError:
    HAS_PILLOW = False
    print("Warning: Pillow not found — thumbnails will be skipped. "
          "Install with: pip install Pillow", file=sys.stderr)

IMAGE_EXTS = {'.jpg', '.jpeg', '.png', '.tif', '.tiff', '.bmp'}
THUMB_SIZE  = (256, 256)

# Pseudo-filesystems to skip entirely
SKIP_PREFIXES = ('/proc', '/sys', '/dev', '/run', '/snap')


# ---------------------------------------------------------------------------
# .lp helpers
# ---------------------------------------------------------------------------

def count_lights(lp_path: str) -> int:
    """Return the number of light entries in an .lp file."""
    try:
        with open(lp_path, 'r', errors='replace') as fh:
            lines = fh.readlines()
        if not lines:
            return 0
        first = lines[0].strip()
        # Common format: first line is a bare integer giving the count
        if re.match(r'^\d+$', first):
            return int(first)
        # Fallback: count non-empty, non-comment lines
        return sum(1 for ln in lines if ln.strip() and not ln.strip().startswith('#'))
    except Exception:
        return 0


# ---------------------------------------------------------------------------
# Image helpers
# ---------------------------------------------------------------------------

def first_image(folder: str):
    """Return the path to the first image file in folder (alphabetical order)."""
    try:
        names = sorted(os.listdir(folder))
    except PermissionError:
        return None
    for name in names:
        if Path(name).suffix.lower() in IMAGE_EXTS:
            return os.path.join(folder, name)
    return None


def make_thumbnail(img_path: str, thumb_path: str) -> bool:
    """Create a padded 256×256 JPEG thumbnail. Returns True on success."""
    if not HAS_PILLOW:
        return False
    try:
        with Image.open(img_path) as im:
            im = im.convert('RGB')
            im.thumbnail(THUMB_SIZE, Image.LANCZOS)
            canvas = Image.new('RGB', THUMB_SIZE, (30, 30, 30))
            offset = ((THUMB_SIZE[0] - im.width) // 2,
                      (THUMB_SIZE[1] - im.height) // 2)
            canvas.paste(im, offset)
            canvas.save(thumb_path, 'JPEG', quality=85)
        return True
    except Exception as exc:
        print(f"  thumbnail failed for {img_path}: {exc}", file=sys.stderr)
        return False


# ---------------------------------------------------------------------------
# Filesystem walk
# ---------------------------------------------------------------------------

def find_lp_folders(root: str):
    """Yield (folder, first_lp_path) for every folder containing an .lp file."""
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        # Prune unwanted subtrees in-place so os.walk skips them
        if any(dirpath.startswith(p) for p in SKIP_PREFIXES):
            dirnames.clear()
            continue
        lp_files = sorted(f for f in filenames if f.lower().endswith('.lp'))
        if lp_files:
            yield dirpath, os.path.join(dirpath, lp_files[0])


# ---------------------------------------------------------------------------
# HTML generation
# ---------------------------------------------------------------------------

def make_row(entry: dict) -> str:
    if entry['thumb']:
        thumb_html = (f'<img src="{entry["thumb"]}" width="256" height="256" '
                      f'loading="lazy" alt="thumbnail">')
    else:
        thumb_html = '<div class="no-thumb">no image</div>'

    return f"""\
    <tr>
      <td class="thumb">{thumb_html}</td>
      <td class="path">{entry['folder']}<br>
          <small class="lp">{entry['lp']}</small></td>
      <td class="lights">{entry['n_lights']}</td>
    </tr>"""


HTML_TEMPLATE = """\
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>LP File Index</title>
<style>
  body  {{ font-family: sans-serif; margin: 20px; background: #1a1a1a; color: #ddd; }}
  h1    {{ color: #fff; margin-bottom: 4px; }}
  p.meta {{ color: #888; font-size: 13px; margin-top: 0; }}
  table {{ border-collapse: collapse; width: 100%; }}
  th    {{ background: #333; color: #ccc; padding: 8px 12px; text-align: left;
           position: sticky; top: 0; z-index: 1; }}
  tr:nth-child(even) {{ background: #222; }}
  tr:nth-child(odd)  {{ background: #1e1e1e; }}
  tr:hover           {{ background: #2a3a4a; }}
  td       {{ padding: 8px 12px; vertical-align: middle; }}
  td.thumb {{ width: 272px; padding: 6px; }}
  td.thumb img {{ display: block; border-radius: 4px; }}
  .no-thumb {{ width: 256px; height: 256px; background: #333; display: flex;
               align-items: center; justify-content: center; color: #666;
               border-radius: 4px; font-size: 12px; }}
  td.path  {{ font-family: monospace; font-size: 13px; word-break: break-all; }}
  td.lights {{ text-align: center; font-size: 22px; font-weight: bold;
               color: #7af; width: 80px; }}
  small.lp {{ color: #666; font-size: 11px; }}
  input#filter {{ padding: 6px 10px; font-size: 14px; background: #333;
                  color: #eee; border: 1px solid #555; border-radius: 4px;
                  width: 420px; margin-bottom: 14px; }}
  span#count {{ color: #888; font-size: 13px; margin-left: 12px; }}
</style>
</head>
<body>
<h1>LP File Index</h1>
<p class="meta">
  Generated {date} &mdash; searched under <code>{search_root}</code>
  &mdash; <strong>{count}</strong> folder(s) found.
</p>
<input id="filter" type="text" placeholder="Filter by path…"
       oninput="filterTable(this.value)">
<span id="count">{count} shown</span>
<table id="tbl">
  <thead><tr><th>Thumbnail</th><th>Folder</th><th>Lights</th></tr></thead>
  <tbody>
{rows}
  </tbody>
</table>
<script>
function filterTable(q) {{
  q = q.toLowerCase();
  let visible = 0;
  document.querySelectorAll('#tbl tbody tr').forEach(tr => {{
    const show = tr.cells[1].textContent.toLowerCase().includes(q);
    tr.style.display = show ? '' : 'none';
    if (show) visible++;
  }});
  document.getElementById('count').textContent = visible + ' shown';
}}
</script>
</body>
</html>
"""


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    search_root = sys.argv[1] if len(sys.argv) > 1 else '/'
    output_dir  = Path(sys.argv[2] if len(sys.argv) > 2 else 'lp_index').resolve()
    thumb_dir   = output_dir / 'thumbs'
    output_dir.mkdir(parents=True, exist_ok=True)
    thumb_dir.mkdir(parents=True, exist_ok=True)

    print(f"Searching under : {search_root}")
    print(f"Output directory: {output_dir}")
    print()

    entries = []
    for folder, lp_path in find_lp_folders(search_root):
        n_lights = count_lights(lp_path)
        img_path = first_image(folder)

        thumb_rel = None
        if img_path:
            safe = re.sub(r'[^a-zA-Z0-9_.-]', '_', folder.lstrip('/')) + '.jpg'
            ok = make_thumbnail(img_path, str(thumb_dir / safe))
            if ok:
                thumb_rel = 'thumbs/' + safe

        entries.append({
            'folder':   folder,
            'lp':       lp_path,
            'n_lights': n_lights,
            'thumb':    thumb_rel,
        })
        thumb_tag = '[thumb]' if thumb_rel else '[     ]'
        print(f"  {len(entries):5d}  {thumb_tag}  {n_lights:4d} lights  {folder}")

    entries.sort(key=lambda e: e['folder'])

    html_path = output_dir / 'index.html'
    with open(html_path, 'w') as fh:
        fh.write(HTML_TEMPLATE.format(
            date=datetime.now().strftime('%Y-%m-%d %H:%M'),
            search_root=search_root,
            count=len(entries),
            rows='\n'.join(make_row(e) for e in entries),
        ))

    print()
    print(f"Done — {len(entries)} folder(s) found.")
    print(f"Open: {html_path}")


if __name__ == '__main__':
    main()
