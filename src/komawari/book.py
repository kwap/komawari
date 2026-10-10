"""The panels file of one prepared comic: what the Vita reader loads."""

import json
from dataclasses import asdict, dataclass, replace
from pathlib import Path
from typing import Any

from komawari.pages import page_numbers
from komawari.shared_format import PageResult, page_from_json

# Raised whenever the structure of the file changes, so a reader can refuse a
# file it does not understand.
FORMAT_VERSION = 1
# Below this share of the page, detected panels are not a real panel layout.
MIN_PANEL_COVERAGE = 0.5


@dataclass(frozen=True)
class Book:
    """One comic: its pages in reading order, each with its panels in reading order."""

    title: str
    reading_direction: str
    detector: str
    pages: list[PageResult]


def without_false_panels(page: PageResult) -> PageResult:
    """Empties the panel list of a page that should be shown whole.

    A detector marks boxes on every page, but three kinds of page have nothing
    worth stepping through:
    - covers and other extras, recognised by a file name without a page number;
    - pages with a single panel, where cutting to it could only lose something;
    - title and text pages, where the boxes cover less than half the page.
    """
    covered = sum(panel.width * panel.height for panel in page.panels)
    is_extra = len(page_numbers(page.file)) == 0
    is_sparse = covered < MIN_PANEL_COVERAGE * page.width * page.height
    if is_extra or len(page.panels) == 1 or is_sparse:
        return replace(page, panels=[])
    return page


def save_book(book: Book, path: Path) -> None:
    data = {"format": FORMAT_VERSION, **asdict(book)}
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def load_book(path: Path) -> Book:
    data: Any = json.loads(path.read_text(encoding="utf-8"))
    if data["format"] != FORMAT_VERSION:
        raise ValueError(f"{path} has format {data['format']}; this tool reads format {FORMAT_VERSION}.")
    return Book(
        title=str(data["title"]),
        reading_direction=str(data["reading_direction"]),
        detector=str(data["detector"]),
        pages=[page_from_json(page) for page in data["pages"]],
    )
