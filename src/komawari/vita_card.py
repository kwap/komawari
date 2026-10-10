"""Finds a PS Vita's memory card connected over USB, and lists, copies and removes comics on it.

In VitaShell's USB mode the Vita's memory card appears on the computer as an
ordinary drive. The reader looks for comics in ux0:data/komawari, which on
that drive is the folder data/komawari.
"""

import json
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Any

# Where the comics live, counted from the top of the memory card.
COMICS_FOLDER = Path("data") / "komawari"
# Where the reader's package is put, to be installed from VitaShell.
READER_PACKAGE = Path("data") / "komawari.vpk"


@dataclass(frozen=True)
class ComicOnCard:
    name: str
    page_count: int
    size_bytes: int


def find_card() -> Path | None:
    """Returns the top folder of a connected Vita memory card, or None if none is found."""
    for places in _mount_places():
        if not places.is_dir():
            continue
        for drive in sorted(places.iterdir()):
            if _looks_like_vita_card(drive):
                return drive
    return None


def _mount_places() -> list[Path]:
    """The folders in which macOS and Linux make connected drives appear."""
    user = Path.home().name
    return [
        Path("/Volumes"),  # macOS
        Path("/run/media") / user,  # Fedora and most current Linux desktops
        Path("/media") / user,  # Ubuntu and Debian
        Path("/media"),
        Path("/mnt"),
    ]


def _looks_like_vita_card(drive: Path) -> bool:
    # Every Vita memory card has these two folders: installed apps and their home-screen pages.
    try:
        return (drive / "app").is_dir() and (drive / "appmeta").is_dir()
    except OSError:
        return False


def comics_on_card(card: Path) -> list[ComicOnCard]:
    """Returns the comics on the card, sorted by name: every folder that holds a panels.json."""
    comics_dir = card / COMICS_FOLDER
    if not comics_dir.is_dir():
        return []
    comics: list[ComicOnCard] = []
    for folder in sorted(comics_dir.iterdir()):
        panels_file = folder / "panels.json"
        if not panels_file.is_file():
            continue
        data: Any = json.loads(panels_file.read_text(encoding="utf-8"))
        comics.append(ComicOnCard(folder.name, len(data["pages"]), folder_size(folder)))
    return comics


def is_on_card(card: Path, name: str) -> bool:
    return (card / COMICS_FOLDER / name / "panels.json").is_file()


def is_same_on_card(comic_dir: Path, card: Path) -> bool:
    """Is the comic on the card the one in comic_dir, or an older preparation of it?

    The two panels.json files tell: preparing a comic again changes that file.
    """
    on_card = card / COMICS_FOLDER / comic_dir.name / "panels.json"
    return on_card.is_file() and on_card.read_bytes() == (comic_dir / "panels.json").read_bytes()


def copy_comic(comic_dir: Path, card: Path) -> None:
    """Copies one prepared comic to the card.

    Pictures that are already there with the right size are left alone, so an
    interrupted copy can simply be run again. panels.json is always written, and
    written last: a changed one can have the same size as the old one, and the
    reader only lists a folder once that file is in it, so it never sees half a comic.
    """
    target_dir = card / COMICS_FOLDER / comic_dir.name
    files = sorted(path for path in comic_dir.rglob("*") if path.is_file())
    files.sort(key=lambda path: path.name == "panels.json")  # False sorts before True
    for number, source in enumerate(files, start=1):
        target = target_dir / source.relative_to(comic_dir)
        already_there = target.is_file() and target.stat().st_size == source.stat().st_size
        if source.name == "panels.json" or not already_there:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        if number % 50 == 0 or number == len(files):
            print(f"  {number} of {len(files)} files", flush=True)


def remove_comic(card: Path, name: str) -> None:
    """Deletes one comic from the card. name must be one of comics_on_card()."""
    if name not in [comic.name for comic in comics_on_card(card)]:
        raise ValueError(f"There is no comic called {name!r} on the card.")
    shutil.rmtree(card / COMICS_FOLDER / name)


def copy_reader(package: Path, card: Path) -> Path:
    """Copies the reader's package to the card and returns where it was put."""
    target = card / READER_PACKAGE
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(package, target)
    return target


def folder_size(folder: Path) -> int:
    return sum(path.stat().st_size for path in folder.rglob("*") if path.is_file())


def free_space(card: Path) -> int:
    return shutil.disk_usage(card).free
