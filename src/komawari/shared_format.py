"""The panel data everything else works with: panels, the pages they are on, and what a detector found."""

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class Panel:
    """One panel rectangle, in pixels, measured from the page's top-left corner."""

    x: int
    y: int
    width: int
    height: int


@dataclass(frozen=True)
class PageResult:
    """The panels found on one page, listed in reading order."""

    file: str
    width: int
    height: int
    panels: list[Panel]


@dataclass(frozen=True)
class DetectionResult:
    """Everything one detector found on a set of pages."""

    detector: str
    pages: list[PageResult]


def page_from_json(page: Any) -> PageResult:
    return PageResult(
        file=str(page["file"]),
        width=int(page["width"]),
        height=int(page["height"]),
        panels=[
            Panel(x=int(p["x"]), y=int(p["y"]), width=int(p["width"]), height=int(p["height"]))
            for p in page["panels"]
        ],
    )
