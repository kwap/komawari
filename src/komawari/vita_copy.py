"""Makes the Vita-sized copy of a prepared comic: smaller pages, a cover picture, a
picture of its title, and a matching panels.json."""

import io
import math
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, replace
from functools import partial
from pathlib import Path

from PIL import Image

from komawari.book import Book, load_book, save_book
from komawari.details import Details
from komawari.load_time import load_seconds
from komawari.shared_format import PageResult, Panel
from komawari.title_picture import write_title_picture

# The Vita cannot hold a picture wider or taller than this (its texture limit).
MAX_WIDTH = 4096
# Height of a normal page. At 2560, 83% of the panels of a manga volume have at least
# as many pixels as the 960 x 544 screen shows when the panel is fitted to it.
# A taller page costs memory and loading time on the Vita.
MAX_HEIGHT = 2560
# Height of the cover picture the reader shows when a comic is being chosen.
# It is the height of the Vita's screen.
COVER_HEIGHT = 544
# The picture of the comic's title, subtitle and author, shown under the cover.
TITLE_PICTURE = "title.png"
# JPEG, because the Vita decodes it several times faster than PNG.
JPEG_QUALITY = 90
# A page that the Vita would take longer than this to load is made smaller, until
# it takes about this long. In a manga volume these are the colour spreads and
# the dust jacket; a normal grey page takes about a second.
SLOWEST_LOAD_SECONDS = 3.0
# Two kinds of picture are then made smaller once more, to this share of their
# width and height, because they are the slowest to load: a picture that is
# wider than tall, such as a two-page spread or a dust jacket, and any picture
# in colour, which costs the Vita three times as much per pixel as a grey one.
# Tried on a Vita: at 0.7 these pictures were sharp enough, also zoomed in, and
# loaded in half the time. The owner then chose 0.8, for a little more
# sharpness; that takes about a third longer than 0.7.
HEAVY_PAGE_SCALE = 0.8
# Only a large scan has those pixels to spare: one at least twice as wide and
# high as the page made from it, which is then very sharp. A smaller scan keeps
# every pixel it brings.
LARGE_SCAN_SCALE = 0.5


@dataclass(frozen=True)
class VitaCopy:
    """What make_vita_copy wrote."""

    book: Book
    pages_made_smaller: int  # pages below the usual size, so that they load faster


@dataclass(frozen=True)
class _SmallPage:
    page: PageResult
    made_smaller: bool


def make_vita_copy(book_dir: Path, target_dir: Path, details: Details, fonts_dir: Path) -> VitaCopy:
    """Writes target_dir from the full-size pages and panels.json in book_dir."""
    book = _with_details(load_book(book_dir / "panels.json"), details)
    source_pages = book_dir / "pages"
    (target_dir / "pages").mkdir(parents=True, exist_ok=True)

    shrink = partial(_shrink_page, source_dir=source_pages, target_dir=target_dir / "pages")
    # Resizing is slow and each page is independent, so several run at once.
    with ThreadPoolExecutor() as pool:
        small_pages = list(pool.map(shrink, book.pages))

    _write_cover(source_pages / _cover_page(book).file, target_dir / "cover.jpg")
    write_title_picture(details, fonts_dir, target_dir / TITLE_PICTURE)
    small_book = replace(book, pages=[small.page for small in small_pages])
    save_book(small_book, target_dir / "panels.json")
    return VitaCopy(book=small_book, pages_made_smaller=sum(small.made_smaller for small in small_pages))


def refresh_details(target_dir: Path, details: Details, fonts_dir: Path) -> bool:
    """Brings the title, subtitle and author of a finished Vita copy up to date.

    Returns False if they already were. panels.json is written again too: a
    changed panels.json is how `komawari transfer` sees that a comic changed.
    """
    panels_file = target_dir / "panels.json"
    book = load_book(panels_file)
    title_picture = target_dir / TITLE_PICTURE
    if book == _with_details(book, details) and title_picture.is_file():
        return False
    write_title_picture(details, fonts_dir, title_picture)
    save_book(_with_details(book, details), panels_file)
    return True


def _with_details(book: Book, details: Details) -> Book:
    return replace(book, title=details.title, subtitle=details.subtitle, author=details.author)


def _shrink_page(page: PageResult, source_dir: Path, target_dir: Path) -> _SmallPage:
    with Image.open(source_dir / page.file) as image:
        plain = _plain(image)
    in_colour = plain.mode != "L"

    scale = min(1.0, MAX_WIDTH / page.width, MAX_HEIGHT / page.height)
    width, height, jpeg = _resized_jpeg(plain, page, scale)
    seconds = load_seconds(width * height, len(jpeg), in_colour)
    is_slow = seconds > SLOWEST_LOAD_SECONDS
    is_heavy = (page.width > page.height or in_colour) and scale <= LARGE_SCAN_SCALE
    if is_slow:
        # Loading time follows the number of pixels, which follows the square of the scale.
        scale *= math.sqrt(SLOWEST_LOAD_SECONDS / seconds)
    if is_heavy:
        scale *= HEAVY_PAGE_SCALE
    made_smaller = is_slow or is_heavy
    if made_smaller:
        width, height, jpeg = _resized_jpeg(plain, page, scale)

    file = f"{Path(page.file).stem}.jpg"
    (target_dir / file).write_bytes(jpeg)
    panels = [_shrink_panel(panel, scale) for panel in page.panels]
    small_page = PageResult(file=file, width=width, height=height, panels=panels)
    return _SmallPage(page=small_page, made_smaller=made_smaller)


def _resized_jpeg(plain: Image.Image, page: PageResult, scale: float) -> tuple[int, int, bytes]:
    """The page at this scale as a JPEG file in memory, with its width and height."""
    width, height = round(page.width * scale), round(page.height * scale)
    small = plain.resize((width, height), Image.Resampling.LANCZOS)
    buffer = io.BytesIO()
    small.save(buffer, "JPEG", quality=JPEG_QUALITY)
    return width, height, buffer.getvalue()


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
