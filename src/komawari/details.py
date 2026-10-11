"""What the reader shows about a comic where it is chosen: its title, subtitle and author.

They come from a small text file next to the archive, with the archive's name
and the ending .txt. For `My Comic 1.cbz` that is `My Comic 1.txt`:

    title: My Comic
    subtitle: Part 1
    author: Some Body

Any of the three lines may be left out. Without the file, the title is the
archive's name and the other two are empty.
"""

from dataclasses import dataclass, replace
from pathlib import Path

FIELDS = ("title", "subtitle", "author")


@dataclass(frozen=True)
class Details:
    title: str
    subtitle: str
    author: str


def details_file(archive: Path) -> Path:
    return archive.with_suffix(".txt")


def read_details(archive: Path) -> Details:
    """Returns the details of the comic in this archive. Raises ValueError for a line it cannot read."""
    details = Details(title=archive.stem, subtitle="", author="")
    file = details_file(archive)
    if not file.is_file():
        return details
    for number, line in enumerate(file.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        field, colon, value = line.partition(":")
        field = field.strip().lower()
        if not colon or field not in FIELDS:
            raise ValueError(
                f'{file}, line {number}: expected "title:", "subtitle:" or "author:" and then the text.'
            )
        details = replace(details, **{field: value.strip()})
    return details
