"""Reads comic archives (CBZ or CBR) with the bsdtar command.

Python can open zip files itself but not RAR files. bsdtar reads both, and it
ships with macOS and with most Linux distributions.
"""

import re
import subprocess
from pathlib import Path

from komawari.pages import is_image

ARCHIVE_SUFFIXES = {".cbz", ".cbr", ".cb7", ".zip", ".rar", ".7z"}


def list_archives(folder: Path) -> list[Path]:
    """Returns the comic archives in the folder, sorted by name."""
    return sorted(
        path
        for path in folder.iterdir()
        if path.is_file() and path.suffix.lower() in ARCHIVE_SUFFIXES and not path.name.startswith(".")
    )


def newest_archive(folder: Path) -> Path | None:
    """Returns the archive that arrived in the folder last, or None if there is none.

    A copied file keeps the dates of the original, so "created" and "modified"
    say nothing about when it arrived. The "changed" time does: the system sets
    it when the copy is made.
    """
    archives = list_archives(folder)
    return max(archives, key=lambda path: path.stat().st_ctime) if archives else None


def list_images(archive: Path) -> list[str]:
    """Returns the names of the images inside the archive, in the archive's own order.

    That order is the reading order: the cover comes first, although its file
    name would sort last.
    """
    return [name for name in _list_entries(archive) if is_image(name)]


def _list_entries(archive: Path) -> list[str]:
    listing = subprocess.run(
        ["bsdtar", "-tf", str(archive)], check=True, capture_output=True, text=True
    )
    return listing.stdout.splitlines()


def stated_reading_direction(archive: Path) -> bool | None:
    """Says whether the archive itself declares the comic to read right to left.

    Returns True for right to left, False for left to right, and None when the
    archive does not say.

    Comic programs can put a small description file, ComicInfo.xml, into an
    archive. Its Manga field is "YesAndRightToLeft" for a comic read right to
    left and "No" for an ordinary left-to-right one. "Yes" and "Unknown" say
    nothing about the direction. Many archives have no such file.
    """
    info_files = [name for name in _list_entries(archive) if Path(name).name.lower() == "comicinfo.xml"]
    if not info_files:
        return None
    info = subprocess.run(
        ["bsdtar", "-xOf", str(archive), info_files[0]],
        check=True,
        capture_output=True,
        encoding="utf-8",
        errors="replace",
    )
    field = re.search(r"<Manga>\s*(\w+)\s*</Manga>", info.stdout)
    if field is None:
        return None
    return {"YesAndRightToLeft": True, "No": False}.get(field.group(1))


def extract(archive: Path, names: list[str], destination: Path) -> None:
    subprocess.run(["bsdtar", "-xf", str(archive), "-C", str(destination), *names], check=True)
