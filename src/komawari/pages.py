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

    Three habits of the people who name these files are allowed for:
    - notes in brackets after the number, as in `... p045 [Digital].png`;
    - a `p` before the number, as in `... p018-p019.png`;
    - the number of the issue before the page, as in `... 003-045.webp`. Two
      numbers are only a spread when the second follows the first.
    """
    stem = re.sub(r"(\s*(\[[^\]]*\]|\([^)]*\)))+$", "", Path(name).stem)
    match = re.search(r"p?(\d+)(?:-p?(\d+))?$", stem)
    if match is None:
        return range(0)
    first = int(match.group(1))
    if match.group(2) is None:
        return range(first, first + 1)
    second = int(match.group(2))
    if second == first + 1:
        return range(first, second + 1)
    return range(second, second + 1)


def in_reading_order(names: list[str]) -> list[str]:
    """Puts the page files of an archive in reading order.

    Most archives list their pages in reading order, and some of those give the
    cover a name that would sort last; so the archive's own order is kept. But
    when every name holds a page number, the numbers decide: some archives list
    their first pages last.
    """
    if all(len(page_numbers(name)) > 0 for name in names):
        # Pages with the same number keep the order they had.
        return sorted(names, key=lambda name: page_numbers(name)[0])
    return names
