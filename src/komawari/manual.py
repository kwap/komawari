"""Builds the manual: one HTML page made from a Markdown file.

The text is written in Markdown (README.md) and stays readable there. This
module turns it into a page with a contents list, in the look of the reader's
artwork, for opening in a browser.

The markdown-it-py library turns Markdown into HTML. It first cuts the text
into a list of "tokens" (a heading opens, some text, the heading closes, ...)
and then writes HTML from that list. This module steps in between: it takes
the title off the top, for the band of the page, and gives each heading a name
that the contents list can link to.
"""

import html
import re
from dataclasses import dataclass
from pathlib import Path

from markdown_it import MarkdownIt
from markdown_it.token import Token

from komawari.artwork import (
    ACCENT,
    APP_NAME_JAPANESE,
    INK,
    INK_LIGHT,
    MARK_LEFT,
    MARK_RIGHT,
    MARK_TOP,
    PAPER,
    TAGLINE_LINES,
    Colour,
    Shape,
)

# Shown under the tagline in the band at the top of the page.
PLATFORM_LINE = "Built for the PS Vita"


@dataclass
class Heading:
    text: str
    anchor: str  # the name links point to: href="#anchor"


def build_manual(source: Path, name_font_url: str) -> str:
    """Returns the whole HTML page. The file's title becomes the title of the page.

    name_font_url is the path from the page to the font file used for that title.
    """
    markdown = MarkdownIt("commonmark").enable("table")
    tokens = markdown.parse(source.read_text(encoding="utf-8"))
    # A heading is three tokens: it opens, its text, it closes.
    if len(tokens) < 3 or tokens[0].type != "heading_open" or tokens[0].tag != "h1":
        raise ValueError(f"{source} must start with its title, as a line like '# Title'")
    title = html.escape(_plain_text(tokens[1]))
    body = tokens[3:]
    sections = _name_headings(body)
    text: str = markdown.renderer.render(body, markdown.options, {})

    contents = "".join(
        f'<li><a href="#{section.anchor}">{html.escape(section.text)}</a></li>\n' for section in sections
    )
    tagline = " ".join(f"<span>{line}</span>" for line in TAGLINE_LINES)
    return f"""<!doctype html>
<!-- Built by `komawari manual` from {source.as_posix()}. Do not change this file; change that one. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}: {" ".join(TAGLINE_LINES)} {PLATFORM_LINE}.</title>
<link rel="icon" href="data:image/svg+xml,{_icon()}">
<style>{_style(name_font_url)}</style>
</head>
<body>
<header>
<div class="band">
{_mark("currentColor", "var(--accent)")}
<div>
<h1>{title}</h1>
<p class="japanese">{APP_NAME_JAPANESE}</p>
</div>
<p class="tagline">{tagline} <span class="platform">{PLATFORM_LINE}</span></p>
</div>
</header>
<div class="layout">
<nav aria-label="Contents">
<ol>
{contents}</ol>
</nav>
<main>
{text}<footer>This page is built from <code>{html.escape(source.as_posix())}</code>
by <code>uv run komawari manual</code>.</footer>
</main>
</div>
</body>
</html>
"""


def _name_headings(tokens: list[Token]) -> list[Heading]:
    """Gives every heading a name to link to, and returns the second-level ones for the contents list."""
    sections: list[Heading] = []
    used_anchors: set[str] = set()
    for position, token in enumerate(tokens):
        if token.type != "heading_open":
            continue
        text = _plain_text(tokens[position + 1])
        anchor = _new_anchor(text, used_anchors)
        token.attrSet("id", anchor)
        if token.tag == "h2":
            sections.append(Heading(text, anchor))
    return sections


def _plain_text(inline: Token) -> str:
    """Returns the words of a heading without their formatting."""
    return "".join(piece.content for piece in inline.children or [] if piece.type in ("text", "code_inline"))


def _new_anchor(text: str, used_anchors: set[str]) -> str:
    """Makes the name of a heading the way GitHub does: "What you need" becomes
    "what-you-need". A link written as [text](#what-you-need) then works both
    in the Markdown file and on the page. A name that is taken gets a number."""
    name = re.sub(r"[^\w\- ]", "", text.lower()).replace(" ", "-")
    anchor = name
    count = 0
    while anchor in used_anchors:
        count += 1
        anchor = f"{name}-{count}"
    used_anchors.add(anchor)
    return anchor


def _hex(colour: Colour) -> str:
    return "#{:02x}{:02x}{:02x}".format(*colour)


def _mark(panels_colour: str, accent_colour: str) -> str:
    """The logo mark as a drawing the browser scales: the same three panels as in artwork.py."""

    def panel(corners: Shape, colour: str) -> str:
        points = " ".join(f"{x},{y}" for x, y in corners)
        return f'<polygon points="{points}" fill="{colour}"/>'

    return (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" aria-hidden="true">'
        f"{panel(MARK_TOP, panels_colour)}{panel(MARK_LEFT, panels_colour)}{panel(MARK_RIGHT, accent_colour)}"
        "</svg>"
    )


def _icon() -> str:
    """The mark as the small picture on the browser's tab, written so it fits inside a link."""
    picture = _mark(_hex(PAPER), _hex(ACCENT)).replace(
        'viewBox="0 0 100 100"', f'viewBox="-14 -14 128 128" style="background: {_hex(INK)}"'
    )
    return picture.replace('"', "'").replace("#", "%23").replace("<", "%3C").replace(">", "%3E")


def _style(name_font_url: str) -> str:
    return (
        STYLE.replace("NAME_FONT_URL", name_font_url)
        .replace("INK_LIGHT", _hex(INK_LIGHT))
        .replace("INK", _hex(INK))
        .replace("PAPER", _hex(PAPER))
        .replace("ACCENT", _hex(ACCENT))
    )


# The words in capitals are replaced by _style with the colours from artwork.py.
STYLE = """
@font-face { font-family: "Komawari name"; src: url("NAME_FONT_URL"); font-display: swap; }
:root {
  color-scheme: light dark;
  --accent: ACCENT;
  --bg: #f8f6f0; --fg: INK; --muted: #6a675f; --line: #ddd9cc; --soft: #ece8dc; --link: #1c6cc4;
  --band: INK;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: INK; --fg: #e6e3da; --muted: #a29e95; --line: #303038; --soft: #1f1f25; --link: #72b3f3;
    --band: INK_LIGHT;
  }
}
* { box-sizing: border-box; }
html { scroll-behavior: smooth; scroll-padding-top: 20px; }
body { margin: 0; background: var(--bg); color: var(--fg); font: 16px/1.6 system-ui, sans-serif; }

/* The band at the top: dark like the reader, with a slanted lower edge like the gutters of the mark. */
header {
  background: var(--band); color: PAPER; padding: 44px 24px 76px;
  clip-path: polygon(0 0, 100% 0, 100% calc(100% - 36px), 0 100%);
}
.band { max-width: 1160px; margin: 0 auto; display: flex; flex-wrap: wrap; align-items: center; gap: 24px; }
.band svg { width: 88px; height: 88px; flex: none; }
.band h1 {
  margin: 0; font: 400 46px/1 "Komawari name", system-ui, sans-serif;
  letter-spacing: 0.02em; text-transform: uppercase;
}
.band .japanese { margin: 12px 0 0; font: 400 20px/1 "Komawari name", system-ui, sans-serif; color: var(--accent); }
.band .tagline { margin: 0 0 0 auto; text-align: right; font-size: 19px; font-weight: 700; line-height: 1.25; }
.band .tagline span { display: block; }
.band .tagline .platform {
  margin-top: 10px; font-size: 14px; letter-spacing: 0.09em; text-transform: uppercase; color: var(--accent);
}

.layout {
  max-width: 1160px; margin: 0 auto; padding: 12px 24px 80px;
  display: grid; grid-template-columns: 220px minmax(0, 1fr); gap: 52px;
}

/* The contents list stays in view while the text scrolls. */
nav { position: sticky; top: 20px; align-self: start; max-height: calc(100vh - 40px); overflow-y: auto;
  font-size: 14px; line-height: 1.35; }
nav ol { list-style: none; margin: 0; padding: 0; border-left: 2px solid var(--line); }
nav li a {
  display: block; margin-left: -2px; padding: 4px 0 4px 14px; border-left: 2px solid transparent;
  color: var(--muted); text-decoration: none;
}
nav li a:hover { color: var(--fg); border-left-color: var(--accent); }

main { min-width: 0; }
main h2 { margin: 52px 0 14px; padding-top: 26px; border-top: 1px solid var(--line); font-size: 23px; line-height: 1.25; }
main h3 { margin: 36px 0 10px; font-size: 17px; }
main p, main ul, main ol, main pre, main table { margin: 0 0 16px; }
main p, main li { max-width: 74ch; }
main li + li { margin-top: 4px; }
main li pre { margin: 10px 0 12px; }
a { color: var(--link); text-decoration-thickness: 1px; text-underline-offset: 3px; }
code {
  font: 0.87em/1.5 ui-monospace, "SF Mono", Menlo, Consolas, monospace;
  background: var(--soft); padding: 0.12em 0.38em; border-radius: 4px;
  font-variant-ligatures: none; /* keeps the two hyphens of --ltr apart */
}
a code { color: inherit; }
pre {
  background: var(--soft); border-left: 3px solid var(--accent); border-radius: 0 6px 6px 0;
  padding: 14px 18px; overflow-x: auto;
}
pre code { background: none; padding: 0; font-size: 13px; }
/* A table is as wide as it needs to be, and scrolls sideways by itself on a narrow screen. */
table { display: block; width: fit-content; max-width: 100%; overflow-x: auto; border-collapse: collapse; font-size: 15px; }
th, td { text-align: left; vertical-align: top; padding: 9px 24px 9px 0; border-bottom: 1px solid var(--line); }
th { font-size: 13px; font-weight: 600; color: var(--muted); }
th:last-child, td:last-child { padding-right: 0; }
td:first-child { white-space: nowrap; }
footer { margin-top: 80px; padding-top: 20px; border-top: 1px solid var(--line); font-size: 13px; color: var(--muted); }

@media (max-width: 860px) {
  .layout { display: block; }
  nav { position: static; max-height: none; margin-bottom: 40px; }
  .band .tagline { margin: 0; flex-basis: 100%; text-align: left; font-size: 17px; }
  .band .tagline span { display: inline; }
  .band .tagline .platform { display: block; margin-top: 6px; }
  .band svg { width: 64px; height: 64px; }
  .band h1 { font-size: 34px; }
}
"""
