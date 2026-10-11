"""Draws the pictures the Vita shows for the reader outside the reader itself.

They are all made from one logo mark, one name and three colours (ink, paper and one accent):
    icon0.png     the round "bubble" on the home screen
    bg.png        the background of the page that opens when the bubble is tapped
    startup.png   the start button on that page
    pic0.png      the loading screen

The Vita only accepts these as PNG files with a palette of at most 256 colours.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

APP_NAME = "KOMAWARI"
APP_NAME_JAPANESE = "コマ割り"
TAGLINE_LINES = ["Comics,", "one panel", "at a time."]

# The colours: ink, with two slightly lighter inks for panels and quiet shapes,
# paper for lettering, and azure as the one accent.
INK = (17, 17, 20)
INK_LIGHT = (25, 25, 30)
INK_LIGHTER = (34, 34, 41)
PAPER = (242, 239, 230)
ACCENT = (61, 143, 230)

Colour = tuple[int, int, int]
Shape = list[tuple[float, float]]

# The logo mark is a page cut into three panels by slanted gutters.
# The corners are given in a box of 100 x 100.
MARK_TOP: Shape = [(0, 0), (100, 0), (100, 40), (0, 48)]
MARK_LEFT: Shape = [(0, 55), (56.5, 50.5), (44.5, 100), (0, 100)]
MARK_RIGHT: Shape = [(63.5, 49.9), (100, 47), (100, 100), (51.5, 100)]

# Everything is drawn this many times too large and then shrunk, which smooths the edges.
OVERSIZE = 4


class Canvas:
    """A picture being drawn. Positions and sizes are given in pixels of the finished picture."""

    def __init__(self, width: int, height: int, background: Colour, fonts_dir: Path) -> None:
        self.width = width
        self.height = height
        self._fonts_dir = fonts_dir
        self._image = Image.new("RGB", (width * OVERSIZE, height * OVERSIZE), background)
        self._draw = ImageDraw.Draw(self._image)

    def shape(self, corners: Shape, colour: Colour) -> None:
        self._draw.polygon([(x * OVERSIZE, y * OVERSIZE) for x, y in corners], fill=colour)

    def mark(self, x: float, y: float, size: float, top: Colour, left: Colour, right: Colour) -> None:
        """Draws the logo mark with its top-left corner at x, y."""
        for corners, colour in ((MARK_TOP, top), (MARK_LEFT, left), (MARK_RIGHT, right)):
            self.shape([(x + cx * size / 100, y + cy * size / 100) for cx, cy in corners], colour)

    def text(self, x: float, y: float, text: str, font_file: str, size: float, colour: Colour, anchor: str) -> None:
        """Writes text. The anchor says which point of the text sits at x, y: "la" is its
        top-left corner, "ma" the middle of its top edge, "ra" its top-right corner."""
        font = ImageFont.truetype(str(self._fonts_dir / font_file), round(size * OVERSIZE))
        self._draw.text((x * OVERSIZE, y * OVERSIZE), text, font=font, fill=colour, anchor=anchor)

    def finished(self) -> Image.Image:
        """Shrinks the picture to its real size and reduces it to a 256-colour palette."""
        small = self._image.resize((self.width, self.height), Image.Resampling.LANCZOS)
        return small.quantize(colors=256, dither=Image.Dither.NONE)


NAME_FONT = "DelaGothicOne-Regular.ttf"
TEXT_FONT = "ZenKakuGothicNew-Bold.ttf"

# The page behind the bubble is laid out as a comic page of three panels with
# slanted gutters, like the logo mark turned on its side: two panels stacked on
# the left and a tall one on the right. Corners are in pixels of the 840 x 500 picture.
#
# The Vita decides where its start button goes: with the layout style
# "psmobile" (see template.xml) it covers x 520 to 800, y 155 to 313. The tall
# panel is drawn around that spot, so the button sits in the middle of it.
#
# The Vita also lays a row of its own round icons over the top edge of the
# page. The name panel is tall, and the name sits low in it, so that those
# icons do not cover the name.
PAGE_NAME: Shape = [(20, 20), (490, 20), (481, 192), (20, 206)]
PAGE_TAGLINE: Shape = [(20, 220), (480.2, 206), (466, 480), (20, 480)]
PAGE_START: Shape = [(504, 20), (820, 20), (820, 480), (480, 480)]


def write_artwork(fonts_dir: Path, target_dir: Path) -> list[Path]:
    """Writes the four pictures into target_dir, laid out as the Vita package needs them."""
    pictures = {
        target_dir / "icon0.png": _bubble(fonts_dir),
        target_dir / "pic0.png": _loading_screen(fonts_dir),
        target_dir / "livearea" / "contents" / "bg.png": _page(fonts_dir),
        target_dir / "livearea" / "contents" / "startup.png": _start_button(fonts_dir),
    }
    for path, picture in pictures.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        picture.save(path)
    return list(pictures)


def _bubble(fonts_dir: Path) -> Image.Image:
    """128 x 128. The Vita shows it cut to a circle, so the mark is drawn larger
    than the picture and its three panels fill the whole circle."""
    canvas = Canvas(128, 128, INK, fonts_dir)
    canvas.mark(-10, -10, 148, PAPER, PAPER, ACCENT)
    return canvas.finished()


def _start_button(fonts_dir: Path) -> Image.Image:
    """280 x 158. The Vita lays its own Start button over the lower part, which is left empty."""
    canvas = Canvas(280, 158, INK, fonts_dir)
    canvas.mark(100, 14, 80, PAPER, PAPER, ACCENT)
    return canvas.finished()


def _page(fonts_dir: Path) -> Image.Image:
    """840 x 500, the page behind the bubble."""
    canvas = Canvas(840, 500, INK, fonts_dir)
    canvas.shape(PAGE_NAME, INK_LIGHT)
    canvas.shape(PAGE_TAGLINE, INK_LIGHT)
    canvas.shape(PAGE_START, ACCENT)

    # Top-left panel: the name, with its Japanese spelling under it. Both sit
    # in the lower part of the panel, clear of the Vita's own icons.
    canvas.text(44, 84, APP_NAME, NAME_FONT, 52, PAPER, "la")
    canvas.text(46, 150, APP_NAME_JAPANESE, NAME_FONT, 24, ACCENT, "la")

    # Bottom-left panel: the tagline, and the Japanese name running down the edge, barely visible.
    for row, character in enumerate(APP_NAME_JAPANESE):
        canvas.text(408, 228 + row * 62, character, NAME_FONT, 60, INK_LIGHTER, "ma")
    for row, line in enumerate(TAGLINE_LINES):
        canvas.text(46, 252 + row * 52, line, TEXT_FONT, 42, PAPER, "la")
    return canvas.finished()


def _loading_screen(fonts_dir: Path) -> Image.Image:
    """960 x 544, the whole screen. Dark, like the reader that follows it."""
    canvas = Canvas(960, 544, INK, fonts_dir)
    size = 128
    canvas.mark((960 - size) / 2, 150, size, PAPER, PAPER, ACCENT)
    canvas.text(480, 310, APP_NAME, NAME_FONT, 44, PAPER, "ma")
    canvas.text(480, 372, APP_NAME_JAPANESE, NAME_FONT, 20, ACCENT, "ma")
    return canvas.finished()


def preview(target_dir: Path) -> Image.Image:
    """Puts the pictures together roughly the way the Vita shows them, for a quick look on the computer."""
    contents = target_dir / "livearea" / "contents"
    sheet = Image.new("RGB", (960 * 2 + 60, 544 + 320), (58, 60, 68))

    # The page that opens from the bubble: background at 60, 44 and start button at 580, 199.
    page = Image.new("RGB", (960, 544), (84, 96, 112))
    page.paste(Image.open(contents / "bg.png").convert("RGB"), (60, 44))
    page.paste(Image.open(contents / "startup.png").convert("RGB"), (580, 199))
    draw = ImageDraw.Draw(page)
    # A stand-in for the Vita's own Start button.
    draw.rounded_rectangle((660, 317, 780, 349), radius=16, fill=(60, 170, 90))
    draw.text((720, 333), "Start", fill=(255, 255, 255), anchor="mm", font_size=18)
    sheet.paste(page, (20, 20))

    sheet.paste(Image.open(target_dir / "pic0.png").convert("RGB"), (1000, 20))

    # The bubble, cut to a circle as on the home screen, at its real size and doubled.
    icon = Image.open(target_dir / "icon0.png").convert("RGB")
    for size, x in ((128, 40), (256, 220)):
        scaled = icon.resize((size, size), Image.Resampling.LANCZOS)
        mask = Image.new("L", (size, size), 0)
        ImageDraw.Draw(mask).ellipse((0, 0, size - 1, size - 1), fill=255)
        sheet.paste(scaled, (x, 584 + (256 - size) // 2), mask)
    return sheet
