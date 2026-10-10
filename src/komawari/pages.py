"""Tells page images from other files and reads page numbers from their names."""

import re
from pathlib import Path

IMAGE_SUFFIXES = {".png", ".jpg", ".jpeg", ".webp"}


def is_image(name: str) -> bool:
    return Path(name).suffix.lower() in IMAGE_SUFFIXES


def page_numbers(name: str) -> range:
    """Returns the printed page numbers a file name stands for.

    `..._045.png` is page 45, and `..._018-019.png` is a spread holding pages 18
    and 19. Names without a number at the end, such as `..._Cover.jpg` and
    `..._000a.jpg`, give an empty range.
    """
    match = re.search(r"(\d+)(?:-(\d+))?$", Path(name).stem)
    if match is None:
        return range(0)
    first = int(match.group(1))
    last = int(match.group(2) or first)
    return range(first, last + 1)
