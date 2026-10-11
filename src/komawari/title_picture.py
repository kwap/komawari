"""Draws the picture of a comic's title, subtitle and author.

The reader shows it under the comic's cover. The reader cannot write text
itself, so the text arrives as a picture, in the fonts of the reader's artwork.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from komawari.artwork import ACCENT, NAME_FONT, PAPER, TEXT_FONT
from komawari.details import Details

# The picture is see-through around the letters. The reader draws it as it is,
# one pixel of the picture on one pixel of the Vita's screen.
WIDTH = 720
HEIGHT = 100
# Where each line starts, counted from the top, and the size of its letters.
TITLE_TOP, TITLE_SIZE = 0, 38
SUBTITLE_TOP, SUBTITLE_SIZE = 50, 22
AUTHOR_TOP, AUTHOR_SIZE = 78, 17
# A line that is too wide gets smaller letters, down to this size.
SMALLEST_SIZE = 14
# The author is written less brightly than the title: 255 is solid, 0 invisible.
AUTHOR_ALPHA = 165


def write_title_picture(details: Details, fonts_dir: Path, file: Path) -> None:
    picture = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(picture)
    name_font, text_font = fonts_dir / NAME_FONT, fonts_dir / TEXT_FONT
    _line(draw, details.title.upper(), name_font, TITLE_SIZE, TITLE_TOP, (*PAPER, 255))
    _line(draw, details.subtitle, text_font, SUBTITLE_SIZE, SUBTITLE_TOP, (*ACCENT, 255))
    _line(draw, details.author, text_font, AUTHOR_SIZE, AUTHOR_TOP, (*PAPER, AUTHOR_ALPHA))
    picture.save(file)


def _line(
    draw: ImageDraw.ImageDraw, text: str, font_file: Path, size: int, top: int, colour: tuple[int, int, int, int]
) -> None:
    """Writes one line in the middle of the picture's width, as large as fits."""
    font = ImageFont.truetype(str(font_file), size)
    while draw.textlength(text, font=font) > WIDTH and size > SMALLEST_SIZE:
        size -= 1
        font = ImageFont.truetype(str(font_file), size)
    # "mt": the point given is the middle of the top edge of the letters.
    draw.text((WIDTH / 2, top), text, font=font, fill=colour, anchor="mt")
