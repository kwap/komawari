"""Builds the manual: one HTML page made from a Markdown file.

The text is written in Markdown (README.md) and stays readable there. This
module turns it into a page in the look of the owner's home page, kwap.io:
heavy narrow capitals, thick rules, and a switch between light and dark.

The markdown-it-py library turns Markdown into HTML. It first cuts the text
into a list of "tokens" (a heading opens, some text, the heading closes, ...)
and then writes HTML from that list. This module steps in between: it takes
the title off the top, for the top of the page, puts the introduction next to
the start button, and gives each heading a name that the contents list can
link to.
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
    MARK_LEFT,
    MARK_RIGHT,
    MARK_TOP,
    PAPER,
    TAGLINE_LINES,
    Colour,
    Shape,
)

# Shown next to the name at the top of the page.
PLATFORM_LINE = "Built for the PS Vita"

# Where the links in the top bar lead.
HOME_URL = "https://kwap.io/"
CODE_URL = "https://github.com/kwap/komawari"


@dataclass
class Heading:
    text: str
    anchor: str  # the name links point to: href="#anchor"


def build_manual(source: Path) -> str:
    """Returns the whole HTML page. The file's title becomes the title of the page."""
    markdown = MarkdownIt("commonmark").enable("table")
    tokens = markdown.parse(source.read_text(encoding="utf-8"))
    # A heading is three tokens: it opens, its text, it closes.
    if len(tokens) < 3 or tokens[0].type != "heading_open" or tokens[0].tag != "h1":
        raise ValueError(f"{source} must start with its title, as a line like '# Title'")
    title = html.escape(_plain_text(tokens[1]))
    body = tokens[3:]
    sections = _name_headings(body)
    # What comes before the first section is the introduction. It goes next to the start button.
    first_section = next(
        (position for position, token in enumerate(body) if token.type == "heading_open" and token.tag == "h2"),
        len(body),
    )
    introduction: str = markdown.renderer.render(body[:first_section], markdown.options, {})
    text: str = markdown.renderer.render(body[first_section:], markdown.options, {})

    contents = "".join(
        f'<li><a href="#{section.anchor}">{html.escape(section.text)}</a></li>\n' for section in sections
    )
    start = (
        f'<a class="go" href="#{sections[0].anchor}">Start here <span aria-hidden="true">→</span></a>'
        if sections
        else ""
    )
    # The last line of the tagline is the coloured second line of the big sentence.
    sentence = f"{' '.join(TAGLINE_LINES[:-1])} <em>{TAGLINE_LINES[-1]}</em>"
    return f"""<!doctype html>
<!-- Built by `komawari manual` from {source.as_posix()}. Do not change this file; change that one. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}: {" ".join(TAGLINE_LINES)} {PLATFORM_LINE}.</title>
<link rel="icon" href="data:image/svg+xml,{_icon()}">
<script>{THEME_SCRIPT}</script>
<style>{STYLE}</style>
</head>
<body>
<div class="page">
<header class="bar">
<a href="{HOME_URL}">kwap.io</a>
<nav>
<a href="{CODE_URL}">Code</a>
<button class="switch" id="switch" type="button" aria-label="Switch between light and dark">dark</button>
</nav>
</header>
<section class="hero">
<div class="tagrow">
{_mark("currentColor", _hex(ACCENT))}
<h1 class="name">{title}</h1>
<p class="roles"><span lang="ja">{APP_NAME_JAPANESE}</span> · {PLATFORM_LINE}</p>
</div>
<p class="lede">{sentence}</p>
<div class="act">
{start}
<div class="lines">
{introduction}</div>
</div>
</section>
<nav class="contents" aria-label="Contents">
<ol>
{contents}</ol>
</nav>
<main>
{_number_steps(text)}</main>
<footer>
<p>This page is built from <code>{html.escape(source.as_posix())}</code> by <code>uv run komawari manual</code>.</p>
<a href="{HOME_URL}">kwap.io ↗</a>
</footer>
</div>
<script>{SWITCH_SCRIPT}</script>
<script>{TERMINAL_SCRIPT}</script>
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


def _number_steps(text: str) -> str:
    """A heading written as "2. Prepare your comics" gets its number set apart as a small label."""
    return re.sub(r'(<h2 id="[^"]*">)(\d+)\. ', r"\1<span>\2</span>", text)


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


# The page follows the system's light or dark setting until the switch is used. The choice is kept under the
# same name as on kwap.io, so it holds across the owner's pages.
THEME_SCRIPT = """
try {
  var saved = localStorage.getItem("theme");
  if (saved === "light" || saved === "dark") document.documentElement.setAttribute("data-theme", saved);
} catch (e) {}
"""

# The switch shows the look it changes to.
SWITCH_SCRIPT = """
(function () {
  var root = document.documentElement;
  var button = document.getElementById("switch");
  function current() {
    var systemIsDark = window.matchMedia("(prefers-color-scheme: dark)").matches;
    return root.getAttribute("data-theme") || (systemIsDark ? "dark" : "light");
  }
  function label() { button.textContent = current() === "dark" ? "light" : "dark"; }
  button.addEventListener("click", function () {
    var next = current() === "dark" ? "light" : "dark";
    root.setAttribute("data-theme", next);
    try { localStorage.setItem("theme", next); } catch (e) {}
    label();
  });
  label();
})();
"""

# Blocks of commands become terminals: a bar with a copy button, and the commands coloured by what each word is.
# Without this script they stay plain blocks in the same colours.
TERMINAL_SCRIPT = r"""
(function () {
  function escape(text) { return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;"); }
  function part(kind, text) { return '<span class="t-' + kind + '">' + escape(text) + "</span>"; }

  // Returns the commands as HTML, each word wrapped by what it is: a comment, a program's name, an option, ...
  function coloured(text) {
    var continued = false;  // the line before ended with a backslash, so this line carries its command on
    return text.split("\n").map(function (line) {
      if (/^\s*#/.test(line)) return part("comment", line);
      // In turn: spaces, a quoted text, a variable, a sign that joins commands, any other run of characters.
      var pieces = /(\s+)|("[^"]*"|'[^']*')|(\$\w+|\$\{[^}]*\})|(\|\||&&|[|;\\])|([^\s"'$|;\\]+)/g;
      var out = "", piece, atStart = true, wantsCommand = !continued;
      while ((piece = pieces.exec(line))) {
        if (piece[1]) { out += piece[1]; atStart = true; continue; }
        if (piece[2]) {
          out += part("string", piece[2]);
          wantsCommand = false;
        } else if (piece[3]) {
          out += part("variable", piece[3]);
          wantsCommand = false;
        } else if (piece[4]) {
          out += part("operator", piece[4]);
          if (piece[4] !== "\\") wantsCommand = true;
        } else {
          var word = piece[5], setting = /^(\w+)=(.*)$/.exec(word);
          if (wantsCommand && atStart && setting) {
            out += part("variable", setting[1]) + part("operator", "=") + escape(setting[2]);
            wantsCommand = false;
          } else if (wantsCommand && atStart && word === "sudo") {
            out += part("keyword", word);  // the program's name comes after it
          } else if (wantsCommand && atStart) {
            out += part("command", word);
            wantsCommand = false;
          } else if (atStart && word.charAt(0) === "-") {
            out += part("option", word);
          } else {
            out += escape(word);
          }
        }
        atStart = false;
      }
      continued = /\\\s*$/.test(line);
      return out;
    }).join("\n");
  }

  document.querySelectorAll("main pre").forEach(function (pre) {
    var text = pre.textContent.replace(/\n+$/, "");
    var terminal = document.createElement("div");
    terminal.className = "terminal";
    terminal.innerHTML =
      '<div class="termbar"><span>terminal</span><button class="copy" type="button">copy</button></div>';
    pre.parentNode.insertBefore(terminal, pre);
    terminal.appendChild(pre);
    pre.innerHTML = coloured(text);
    // The button copies the commands without the comment lines.
    terminal.querySelector(".copy").addEventListener("click", function () {
      var button = this;
      var commands = text.split("\n").filter(function (line) {
        return line.trim() !== "" && line.trim().charAt(0) !== "#";
      });
      if (!navigator.clipboard) return;
      navigator.clipboard.writeText(commands.join("\n")).then(function () {
        button.textContent = "copied";
        setTimeout(function () { button.textContent = "copy"; }, 2000);
      });
    });
  });
})();
"""

# The fonts are in the fonts folder next to the page; their licences are there too.
STYLE = """
@font-face {
  font-family: "Bricolage Grotesque"; font-weight: 500 800; font-stretch: 75% 100%; font-display: swap;
  src: url("fonts/BricolageGrotesque.woff2") format("woff2");
}
@font-face {
  font-family: "IBM Plex Mono"; font-weight: 500; font-display: swap;
  src: url("fonts/IBMPlexMono-Medium.woff2") format("woff2");
}
:root {
  --bg: #f1eee6; --ink: #0d0d0d; --dim: #57544c; --line: #cbc7ba; --soft: #e3dfd3;
  --accent: #4b2ff2; --on-accent: #ffffff;
  --rule: 3px; --edge: 18px; --pad: 28px;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #0d0d0d; --ink: #f1eee6; --dim: #a5a299; --line: #3a3934; --soft: #22211e;
    --accent: #cdfa3c; --on-accent: #0d0d0d;
  }
}
:root[data-theme="dark"] {
  --bg: #0d0d0d; --ink: #f1eee6; --dim: #a5a299; --line: #3a3934; --soft: #22211e;
  --accent: #cdfa3c; --on-accent: #0d0d0d;
}
/* Blocks of commands use the colours of the Tokyo Night theme: its light variant on the light page and its
   dark one on the dark page. */
:root {
  --t-bg: #e6e7ed; --t-bar: #d6d8df; --t-fg: #343b59; --t-comment: #6c6e75; --t-command: #2959aa;
  --t-keyword: #65359d; --t-option: #8f5e15; --t-string: #385f0d; --t-variable: #8c4351; --t-operator: #006c86;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --t-bg: #1a1b26; --t-bar: #16161e; --t-fg: #a9b1d6; --t-comment: #646e9c; --t-command: #7aa2f7;
    --t-keyword: #bb9af7; --t-option: #e0af68; --t-string: #9ece6a; --t-variable: #f7768e; --t-operator: #89ddff;
  }
}
:root[data-theme="dark"] {
  --t-bg: #1a1b26; --t-bar: #16161e; --t-fg: #a9b1d6; --t-comment: #646e9c; --t-command: #7aa2f7;
  --t-keyword: #bb9af7; --t-option: #e0af68; --t-string: #9ece6a; --t-variable: #f7768e; --t-operator: #89ddff;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
html { font-size: 18px; scroll-behavior: smooth; scroll-padding-top: 20px; }
body {
  background: var(--bg); color: var(--ink);
  font-family: "Bricolage Grotesque", system-ui, sans-serif; font-weight: 500; line-height: 1.5;
  -webkit-font-smoothing: antialiased; transition: background 0.25s, color 0.25s;
}
a { color: inherit; }
.page { max-width: 1280px; margin: 0 auto; padding: 0 var(--pad) 64px; }

/* Heavy narrow capitals */
.name, .lede, .go, h2, h3 {
  font-weight: 800; font-stretch: 75%; text-transform: uppercase;
  letter-spacing: 0.005em; word-spacing: 0.08em; line-height: 0.88;
}
/* Labels */
.bar, .roles, .contents, h2 span, th, .termbar, footer a {
  font-family: "IBM Plex Mono", ui-monospace, monospace; font-size: 0.72rem; font-weight: 500; font-stretch: 100%;
  letter-spacing: 0.12em; word-spacing: 0; text-transform: uppercase; line-height: 1.5;
}

/* Top bar */
.bar {
  display: flex; justify-content: space-between; align-items: center;
  padding: var(--edge) 0; border-bottom: var(--rule) solid var(--ink);
}
.bar a { text-decoration: none; }
.bar a:hover { text-decoration: underline; }
.bar nav { display: flex; gap: 26px; align-items: center; }
.switch {
  font: inherit; letter-spacing: inherit; text-transform: inherit;
  background: var(--ink); color: var(--bg); border: 0; padding: 8px 12px; cursor: pointer;
}
.switch:hover { background: var(--accent); color: var(--on-accent); }

/* Hero: the mark and the name as a tag, the tagline as a very large sentence, then where to start.
   cqw is 1% of the hero's width. */
.hero { container-type: inline-size; padding-top: 36px; }
.tagrow { display: flex; align-items: center; flex-wrap: wrap; gap: 12px 18px; }
.tagrow svg { width: 42px; height: 42px; flex: none; }
.name { background: var(--accent); color: var(--on-accent); font-size: 1.5rem; line-height: 1; padding: 7px 12px 8px; }
.roles { color: var(--dim); }
.lede {
  font-size: min(13.8vw, 188px);  /* for browsers that do not know cqw */
  font-size: min(100cqw / 6.5, 199px);  /* the first line is 6.42 times the font size wide, so it always fits */
  font-optical-sizing: none;  /* one drawing of the letters at every size, which keeps that width true */
  line-height: 0.84; white-space: nowrap; margin-top: 30px; margin-left: -0.02em;
}
.lede em { display: block; font-style: normal; color: var(--accent); }
.act {
  display: flex; align-items: center; flex-wrap: wrap; gap: 16px 30px;
  margin-top: 40px; padding: 20px 0;
  border-top: var(--rule) solid var(--ink); border-bottom: var(--rule) solid var(--ink);
}
.go {
  display: inline-flex; gap: 12px; flex: none; text-decoration: none; background: var(--ink); color: var(--bg);
  font-size: 1.6rem; line-height: 1; padding: 18px 24px 20px; transition: background 0.15s, color 0.15s;
}
.go:hover { background: var(--accent); color: var(--on-accent); }
.lines { flex: 1 1 22em; font-size: 1.05rem; line-height: 1.45; }
.lines p { max-width: 44em; }
.lines p + p { margin-top: 4px; }

/* The contents list: a row of boxes */
.contents { margin-top: 20px; }
.contents ol { list-style: none; display: flex; flex-wrap: wrap; gap: 10px; }
.contents a { display: block; text-decoration: none; border: var(--rule) solid var(--ink); padding: 7px 12px; }
.contents a:hover { background: var(--accent); border-color: var(--accent); color: var(--on-accent); }

/* The text: each section's heading sits on a thick rule */
main h2 {
  display: flex; align-items: baseline; gap: 14px; margin: 80px 0 24px; padding-bottom: 12px;
  border-bottom: var(--rule) solid var(--ink); font-size: 1.6rem; line-height: 1;
}
main h2 span { color: var(--dim); }
main h3 { font-size: 1.25rem; margin: 36px 0 12px; }
main p, main ul, main ol, main pre, main table { margin-bottom: 16px; }
main p, main li { max-width: 68ch; }
main ul, main ol { padding-left: 1.4em; }
main li + li { margin-top: 6px; }
main li pre, main li .terminal { margin: 10px 0 12px; }
main a, .lines a { text-decoration-thickness: 2px; text-underline-offset: 3px; }
main a:hover, .lines a:hover { color: var(--accent); }
code {
  font-family: "IBM Plex Mono", ui-monospace, monospace; font-size: 0.84em;
  background: var(--soft); padding: 0.12em 0.4em;
  font-variant-ligatures: none; /* keeps the two hyphens of --ltr apart */
}
/* A block of commands. The script at the end of the page puts each one in a "terminal" with a bar on top. */
pre {
  max-width: 52rem; border: var(--rule) solid var(--ink); background: var(--t-bg); color: var(--t-fg);
  font-family: "IBM Plex Mono", ui-monospace, monospace; font-size: 0.8rem; font-variant-ligatures: none;
  line-height: 1.6; padding: 14px 18px; overflow-x: auto;
}
pre code { font: inherit; background: none; padding: 0; }
.terminal { max-width: 52rem; margin-bottom: 16px; border: var(--rule) solid var(--ink); background: var(--t-bg); }
.terminal pre { max-width: none; margin: 0; border: 0; }
.termbar {
  display: flex; justify-content: space-between; align-items: center; padding: 6px 6px 6px 14px;
  background: var(--t-bar); color: var(--t-comment);
}
.copy {
  font: inherit; letter-spacing: inherit; text-transform: inherit;
  background: var(--t-command); color: var(--t-bg); border: 0; padding: 5px 10px; cursor: pointer;
}
.copy:hover { background: var(--t-fg); }
.t-comment { color: var(--t-comment); }
.t-command { color: var(--t-command); }
.t-keyword { color: var(--t-keyword); }
.t-option { color: var(--t-option); }
.t-string { color: var(--t-string); }
.t-variable { color: var(--t-variable); }
.t-operator { color: var(--t-operator); }
/* A table is as wide as it needs to be, and scrolls sideways by itself on a narrow screen. */
table { display: block; width: fit-content; max-width: 100%; overflow-x: auto; border-collapse: collapse; }
th, td { text-align: left; vertical-align: top; padding: 10px 28px 10px 0; border-bottom: 1px solid var(--line); }
th { color: var(--dim); border-bottom: var(--rule) solid var(--ink); }
th:last-child, td:last-child { padding-right: 0; }
td:first-child { white-space: nowrap; }

footer {
  margin-top: 88px; padding-top: 18px; border-top: var(--rule) solid var(--ink);
  display: flex; justify-content: space-between; align-items: baseline; flex-wrap: wrap; gap: 12px 24px;
  font-size: 0.8rem; color: var(--dim);
}
footer a { color: var(--ink); text-decoration: none; }
footer a:hover { text-decoration: underline; }

@media (max-width: 860px) {
  html { font-size: 17px; }
  :root { --pad: 16px; }
  .page { padding-bottom: 48px; }
  .bar nav { gap: 14px; }
  .hero { padding-top: 26px; }
  .lede { margin-top: 22px; }
  .act { margin-top: 28px; }
  .go { font-size: 1.35rem; }
  main h2 { margin-top: 60px; }
}
/* On a phone a table row becomes two lines: the first cell, then the second under it. */
@media (max-width: 600px) {
  table { width: 100%; }
  thead { display: none; }
  tr { display: block; padding: 10px 0; border-bottom: 1px solid var(--line); }
  td { display: block; padding: 0; border: 0; }
  td:first-child { white-space: normal; font-weight: 700; margin-bottom: 2px; }
  td code { white-space: nowrap; }
}
"""
