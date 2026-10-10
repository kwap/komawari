"""Makes the Vita-sized copy of a prepared comic: smaller pages, a cover picture and a matching panels.json."""

from concurrent.futures import ThreadPoolExecutor
from dataclasses import replace
from functools import partial
from pathlib import Path

from PIL import Image

from komawari.book import Book, load_book, save_book
from komawari.shared_format import PageResult, Panel

# The Vita cannot hold a picture wider or taller than this (its texture limit).
MAX_WIDTH = 4096
# Height of a normal page. At 2560, 83% of the panels of a manga volume have at least
# as many pixels as the 960 x 544 screen shows when the panel is fitted to it.
# A taller page costs memory and loading time on the Vita.
MAX_HEIGHT = 2560
# Height of the cover picture the reader shows when a comic is being chosen.
# It is the height of the Vita's screen.
COVER_HEIGHT = 544
# JPEG, because the Vita decodes it several times faster than PNG.
JPEG_QUALITY = 90


def make_vita_copy(book_dir: Path, target_dir: Path) -> Book:
    """Writes target_dir from the full-size pages and panels.json in book_dir."""
    book = load_book(book_dir / "panels.json")
    source_pages = book_dir / "pages"
    (target_dir / "pages").mkdir(parents=True, exist_ok=True)

    shrink = partial(_shrink_page, source_dir=source_pages, target_dir=target_dir / "pages")
    # Resizing is slow and each page is independent, so several run at once.
    with ThreadPoolExecutor() as pool:
        pages = list(pool.map(shrink, book.pages))

    _write_cover(source_pages / _cover_page(book).file, target_dir / "cover.jpg")
    small_book = replace(book, pages=pages)
    save_book(small_book, target_dir / "panels.json")
    return small_book


def _shrink_page(page: PageResult, source_dir: Path, target_dir: Path) -> PageResult:
    scale = min(1.0, MAX_WIDTH / page.width, MAX_HEIGHT / page.height)
    width, height = round(page.width * scale), round(page.height * scale)
    file = f"{Path(page.file).stem}.jpg"
    with Image.open(source_dir / page.file) as image:
        small = _plain(image).resize((width, height), Image.Resampling.LANCZOS)
    small.save(target_dir / file, quality=JPEG_QUALITY)
    panels = [_shrink_panel(panel, scale) for panel in page.panels]
    return PageResult(file=file, width=width, height=height, panels=panels)


def _shrink_panel(panel: Panel, scale: float) -> Panel:
    # Scale the edges, not the size, so neighbouring panels still meet.
    left, top = round(panel.x * scale), round(panel.y * scale)
    right = round((panel.x + panel.width) * scale)
    bottom = round((panel.y + panel.height) * scale)
    return Panel(x=left, y=top, width=right - left, height=bottom - top)


def _cover_page(book: Book) -> PageResult:
    """The first upright page. The very first page is often a wide scan of the whole dust jacket."""
    upright = [page for page in book.pages if page.height > page.width]
    return upright[0] if upright else book.pages[0]


def _write_cover(page_file: Path, cover_file: Path) -> None:
    with Image.open(page_file) as image:
        width = round(image.width * COVER_HEIGHT / image.height)
        cover = _plain(image).resize((width, COVER_HEIGHT), Image.Resampling.LANCZOS)
    cover.save(cover_file, quality=JPEG_QUALITY)


def _plain(image: Image.Image) -> Image.Image:
    """Greyscale pages stay greyscale, which halves the work for the Vita; everything else becomes colour."""
    return image.convert("L" if image.mode == "L" else "RGB")
