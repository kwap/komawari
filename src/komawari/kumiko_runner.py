"""Runs Kumiko on page images and converts its output to the shared format.

Kumiko is not an installable package, so it is run as a separate command from
its clone in third_party/kumiko, and its JSON output is read back.
"""

import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from functools import partial
from pathlib import Path
from typing import Any

from PIL import Image

from komawari.shared_format import DetectionResult, PageResult, Panel


def run_kumiko(page_files: list[Path], kumiko_script: Path, right_to_left: bool) -> DetectionResult:
    """Finds the panels on each page. right_to_left decides the order the panels are put in."""
    # Each page is a separate Kumiko process, so several can run at the same time.
    # The pool keeps the results in the order of page_files.
    workers = max(1, (os.cpu_count() or 2) // 2)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        detect = partial(_detect_page, kumiko_script=kumiko_script, right_to_left=right_to_left)
        outcomes = list(pool.map(detect, page_files))

    failures = [(page.file, error) for page, error in outcomes if error is not None]
    if failures:
        print(f"Kumiko failed on {len(failures)} pages. They get no panels and will be shown whole:")
        for file, error in failures:
            print(f"  {file}: {error}")
    return DetectionResult(detector="kumiko", pages=[page for page, _ in outcomes])


def _detect_page(
    page_file: Path, kumiko_script: Path, right_to_left: bool
) -> tuple[PageResult, str | None]:
    """Returns the page with its panels, and Kumiko's error message if it failed."""
    # sys.executable is the Python of this environment, which has OpenCV installed.
    command = [sys.executable, str(kumiko_script), "-i", str(page_file)]
    if right_to_left:
        command.append("--rtl")
    completed = subprocess.run(command, capture_output=True, text=True)
    if completed.returncode != 0:
        # Kumiko crashes on a few pages. One bad page must not stop a whole comic.
        error = (completed.stderr.strip().splitlines() or ["no error message"])[-1]
        return _page_without_panels(page_file), error

    # Kumiko prints a list with one entry per page; panels are [x, y, width, height].
    info: Any = json.loads(completed.stdout)[0]
    panels = [
        Panel(x=int(x), y=int(y), width=int(width), height=int(height))
        for x, y, width, height in info["panels"]
    ]
    print(f"  {page_file.name}: {len(panels)} panels")
    page = PageResult(
        file=page_file.name,
        width=int(info["size"][0]),
        height=int(info["size"][1]),
        panels=panels,
    )
    return page, None


def _page_without_panels(page_file: Path) -> PageResult:
    with Image.open(page_file) as image:
        return PageResult(file=page_file.name, width=image.width, height=image.height, panels=[])
