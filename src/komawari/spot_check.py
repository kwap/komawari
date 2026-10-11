"""Shows what the panel detection does on a few random pages of a comic, as an HTML page.

A quick look before spending minutes on a whole comic: are the panels found,
and is the reading order right?
"""

import random
import shutil
from pathlib import Path

from komawari.archive import extract, list_images
from komawari.book import without_false_panels
from komawari.kumiko_runner import run_kumiko
from komawari.pages import page_numbers
from komawari.previews import write_previews
from komawari.report import build_report


def spot_check(archive: Path, work_dir: Path, kumiko_script: Path, count: int, right_to_left: bool) -> Path:
    """Checks count random pages of the archive and returns the HTML file that shows them.

    work_dir is emptied first; it holds only the pages and pictures of the last check.
    """
    names = list_images(archive)
    # Prefer numbered pages: covers and other extras are shown whole anyway.
    numbered = [name for name in names if len(page_numbers(name)) > 0]
    candidates = numbered or names
    wanted = set(random.sample(candidates, min(count, len(candidates))))
    chosen = [name for name in names if name in wanted]  # back in reading order

    shutil.rmtree(work_dir, ignore_errors=True)
    pages_dir = work_dir / "pages"
    pages_dir.mkdir(parents=True)
    extract(archive, chosen, pages_dir)
    page_files = [pages_dir / Path(name).name for name in chosen]

    found = run_kumiko(page_files, kumiko_script, right_to_left)
    # The same clean-up as `komawari prepare`, so this shows what the reader will show.
    shown = [without_false_panels(page) for page in found.pages]

    write_previews(page_files, work_dir / "previews")
    report_file = work_dir / "report.html"
    title = f"{archive.stem}: {len(chosen)} random pages"
    report_file.write_text(build_report(shown, previews_url="previews", title=title), encoding="utf-8")
    return report_file
