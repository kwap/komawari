"""Builds a local HTML page that shows the panels found on some pages, next to the pages themselves.

The page shows the reduced copies made by previews.py. Panel rectangles stay in
full-size page pixels; the browser scales the picture and the marks together.
"""

import html
import math
from urllib.parse import quote

from komawari.previews import preview_name
from komawari.shared_format import PageResult, Panel

# The colour of the marks drawn over a page.
COLOUR = "#3d8fe6"
# The largest size of one panel in the reading-sequence strip, in screen pixels.
CROP_MAX_WIDTH = 300
CROP_MAX_HEIGHT = 240

STYLE = """
:root { color-scheme: light dark; --bg: #f5f5f2; --fg: #1c1c1c; --muted: #666; --line: #d6d6d0; }
@media (prefers-color-scheme: dark) {
  :root { --bg: #171717; --fg: #e9e9e6; --muted: #a3a3a3; --line: #3a3a3a; }
}
body { margin: 0; background: var(--bg); color: var(--fg); font: 15px/1.45 system-ui, sans-serif; }
main { max-width: 1500px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { font-size: 22px; margin: 0 0 6px; }
h2 { font-size: 17px; margin: 0 0 12px; }
h2 small, .muted { color: var(--muted); font-weight: normal; font-size: 13px; }
h3 { font-size: 13px; font-weight: 600; margin: 16px 0 6px; }
p { margin: 0 0 8px; max-width: 80ch; }
a { color: inherit; }
table { border-collapse: collapse; margin: 16px 0 0; font-size: 14px; }
th, td { text-align: left; padding: 4px 16px 4px 0; border-bottom: 1px solid var(--line); }
td.count { font-variant-numeric: tabular-nums; }
section { border-top: 1px solid var(--line); margin-top: 32px; padding-top: 20px; }
figure { margin: 0; }
figcaption { font-size: 13px; color: var(--muted); margin-bottom: 6px; }
.columns { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; }
.columns svg { display: block; width: 100%; height: auto; background: #fff; border: 1px solid var(--line); }
.strip { display: flex; gap: 10px; align-items: flex-end; overflow-x: auto; padding-bottom: 8px; }
.strip figure { flex: none; }
.strip svg { display: block; background: #fff; border: 2px solid """ + COLOUR + """; }
.strip figcaption { text-align: center; margin: 4px 0 0; }
"""

# The arrow head used by every arrow on the page.
ARROW_HEAD = (
    '<svg width="0" height="0" style="position: absolute"><defs>'
    '<marker id="arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="4" markerHeight="4" orient="auto">'
    f'<path d="M0,0 L10,5 L0,10 z" fill="{COLOUR}"/></marker></defs></svg>'
)


def build_report(pages: list[PageResult], previews_url: str, title: str) -> str:
    """Returns the whole HTML page. previews_url is the path from the report to the reduced pages."""
    sections = "".join(
        _page_section(index, page, f"{previews_url}/{quote(preview_name(page.file))}")
        for index, page in enumerate(pages)
    )
    summary_rows = "".join(
        f'<tr><td><a href="#page-{index}">{html.escape(page.file)}</a></td>'
        f'<td class="count">{_panel_count(page)}</td></tr>\n'
        for index, page in enumerate(pages)
    )
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<style>{STYLE}</style>
</head>
<body>
{ARROW_HEAD}
<main>
<h1>{html.escape(title)}</h1>
<p>Each page is shown as it is in the comic, then with the panels found on it.
The number is a panel's place in the reading order, and the arrows lead from each panel to the next.
Art that is not shaded was not put in any panel. A page without panels is shown whole by the reader.</p>
<p>Under each page, the panels are cut out in that order, the way the reader steps through them.
Pages are shown at reduced size.</p>
<table>
<tr><th>Page</th><th>Panels</th></tr>
{summary_rows}
</table>
{sections}
</main>
</body>
</html>
"""


def _panel_count(page: PageResult) -> str:
    return f"{len(page.panels)} panels" if page.panels else "no panels, shown whole"


def _page_section(index: int, page: PageResult, image_url: str) -> str:
    strip = ""
    if page.panels:
        crops = "".join(
            _crop(page, panel, order, image_url) for order, panel in enumerate(page.panels, start=1)
        )
        strip = f'<h3>Reading sequence</h3><div class="strip">{crops}</div>'
    return f"""<section id="page-{index}">
<h2>{html.escape(page.file)} <small>{page.width} × {page.height}</small></h2>
<div class="columns">
<figure><figcaption>Original</figcaption>{_page_svg(page, image_url, "")}</figure>
<figure><figcaption>{_panel_count(page)}</figcaption>{_page_svg(page, image_url, _marks(page))}</figure>
</div>
{strip}
</section>
"""


def _page_svg(page: PageResult, image_url: str, marks: str) -> str:
    return (
        f'<svg viewBox="0 0 {page.width} {page.height}">'
        f'<image href="{image_url}" width="{page.width}" height="{page.height}"/>{marks}</svg>'
    )


def _marks(page: PageResult) -> str:
    """Draws the panel rectangles, their order numbers and the arrows between them."""
    # Sizes follow the page width so the marks look the same on small and large pages.
    radius = page.width * 0.035
    line = page.width * 0.005
    rectangles = "".join(
        f'<rect x="{panel.x}" y="{panel.y}" width="{panel.width}" height="{panel.height}" '
        f'fill="{COLOUR}" fill-opacity="0.16" stroke="{COLOUR}" stroke-width="{line:.1f}"/>'
        for panel in page.panels
    )
    arrows = "".join(
        _arrow(current, following, radius, line) for current, following in zip(page.panels, page.panels[1:])
    )
    badges = "".join(_badge(panel, order, radius, line) for order, panel in enumerate(page.panels, start=1))
    return rectangles + arrows + badges


def _centre(panel: Panel) -> tuple[float, float]:
    return panel.x + panel.width / 2, panel.y + panel.height / 2


def _arrow(current: Panel, following: Panel, radius: float, line: float) -> str:
    start_x, start_y = _centre(current)
    end_x, end_y = _centre(following)
    length = math.hypot(end_x - start_x, end_y - start_y)
    if length < radius * 3:
        return ""
    # Start and stop at the edge of the number badges, not under them.
    step_x, step_y = (end_x - start_x) / length, (end_y - start_y) / length
    gap = radius * 1.15
    return (
        f'<line x1="{start_x + step_x * gap:.1f}" y1="{start_y + step_y * gap:.1f}" '
        f'x2="{end_x - step_x * gap:.1f}" y2="{end_y - step_y * gap:.1f}" '
        f'stroke="{COLOUR}" stroke-width="{line * 1.6:.1f}" marker-end="url(#arrow)"/>'
    )


def _badge(panel: Panel, order: int, radius: float, line: float) -> str:
    x, y = _centre(panel)
    return (
        f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{radius:.1f}" fill="{COLOUR}" '
        f'stroke="#fff" stroke-width="{line:.1f}"/>'
        f'<text x="{x:.1f}" y="{y:.1f}" font-size="{radius * 1.2:.1f}" font-family="system-ui, sans-serif" '
        f'font-weight="700" fill="#fff" text-anchor="middle" dominant-baseline="central">{order}</text>'
    )


def _crop(page: PageResult, panel: Panel, order: int, image_url: str) -> str:
    """Shows one panel cut out of the page by pointing an SVG's view at its rectangle."""
    if panel.width <= 0 or panel.height <= 0:
        return f"<figure><figcaption>{order} (empty)</figcaption></figure>"
    scale = min(CROP_MAX_WIDTH / panel.width, CROP_MAX_HEIGHT / panel.height)
    width = max(1, round(panel.width * scale))
    height = max(1, round(panel.height * scale))
    return (
        f'<figure><svg width="{width}" height="{height}" '
        f'viewBox="{panel.x} {panel.y} {panel.width} {panel.height}">'
        f'<image href="{image_url}" width="{page.width}" height="{page.height}"/></svg>'
        f"<figcaption>{order}</figcaption></figure>"
    )
