"""Command line for komawari.

Everyday commands:
    komawari prepare             make every comic in comics/ ready for the Vita
    komawari test                check the panel detection on 5 random pages of the newest comic

With a Vita connected over USB (VitaShell's USB mode):
    komawari install             copy the reader itself to the Vita
    komawari transfer            copy new and changed comics to the Vita
    komawari list                show the comics on the Vita
    komawari remove NAME         delete a comic from the Vita

For working on the project:
    komawari artwork             redraw the reader's pictures for the Vita's home screen
    komawari manual              rebuild index.html, the manual, from README.md
"""

import argparse
import shutil
import sys
from pathlib import Path

from komawari.archive import (
    extract,
    list_archives,
    list_images,
    newest_archive,
    stated_reading_direction,
)
from komawari.artwork import preview, write_artwork
from komawari.book import Book, load_book, save_book, without_false_panels
from komawari.details import details_file, read_details
from komawari.kumiko_runner import run_kumiko
from komawari.manual import build_manual
from komawari.release import LATEST_PACKAGE_URL, download_latest_package
from komawari.spot_check import spot_check
from komawari.vita_card import (
    COMICS_FOLDER,
    comics_on_card,
    copy_comic,
    copy_reader,
    find_card,
    folder_size,
    free_space,
    is_on_card,
    is_same_on_card,
    remove_comic,
)
from komawari.vita_copy import MAX_HEIGHT, MAX_WIDTH, make_vita_copy, refresh_details

# All paths are relative to the repository root, where the commands are run.
KUMIKO_SCRIPT = Path("third_party/kumiko/kumiko")

# Making comics for the Vita.
COMICS_DIR = Path("comics")  # the comic archives, as they were copied in
BOOKS_DIR = Path("books")  # per comic: the full-size pages and their panels.json
FOR_VITA_DIR = Path("for_vita")  # per comic: the folder that goes to the Vita
READER_PACKAGE = Path("build/vita/komawari.vpk")  # the reader itself, if it was built on this computer
DOWNLOADED_PACKAGE = Path("build/downloaded/komawari.vpk")  # the reader as downloaded from the latest release

# Checking the panel detection on a few pages.
TEST_DIR = Path("test")
TEST_PAGE_COUNT = 5

# The reader's pictures for the Vita's home screen.
ARTWORK_FONTS_DIR = Path("reader/artwork")
ARTWORK_DIR = Path("reader/sce_sys")
ARTWORK_PREVIEW = Path("build/artwork-preview.png")

# The manual: one page, built from the user's guide.
MANUAL_SOURCE = Path("README.md")
MANUAL_PAGE = Path("index.html")


def main() -> None:
    args = _build_parser().parse_args()
    if args.command == "prepare":
        prepare_comics(args.archives, asked_direction=args.right_to_left, force=args.force)
    elif args.command == "test":
        test_comic(args.archive, asked_direction=args.right_to_left)
    elif args.command == "install":
        install_reader(args.card)
    elif args.command == "transfer":
        transfer_comics(args.card)
    elif args.command == "list":
        list_comics(args.card)
    elif args.command == "remove":
        remove_from_vita(args.name, args.card)
    elif args.command == "artwork":
        draw_artwork()
    else:
        write_manual()


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="komawari", description="Prepare comics for the Komawari reader on the PS Vita."
    )
    commands = parser.add_subparsers(dest="command", required=True)
    card_help = "the top folder of the Vita's memory card, if it is not found by itself"

    prepare = commands.add_parser(
        "prepare",
        help=f"make every comic in {COMICS_DIR}/ ready for the Vita, in {FOR_VITA_DIR}/; skips those already done",
    )
    prepare.add_argument(
        "archives", nargs="*", type=Path, help=f"only these archives instead of all in {COMICS_DIR}/"
    )
    _add_direction_options(prepare)
    prepare.add_argument("--force", action="store_true", help="prepare again from scratch, even if already done")

    test = commands.add_parser(
        "test",
        help=f"check the panel detection on {TEST_PAGE_COUNT} random pages of the newest comic; writes {TEST_DIR}/report.html",
    )
    test.add_argument("archive", nargs="?", type=Path, help=f"this archive instead of the newest in {COMICS_DIR}/")
    _add_direction_options(test)

    install = commands.add_parser("install", help="copy the reader itself to a Vita connected over USB")
    install.add_argument("--card", type=Path, help=card_help)

    transfer = commands.add_parser("transfer", help="copy new and changed comics to a Vita connected over USB")
    transfer.add_argument("--card", type=Path, help=card_help)

    list_ = commands.add_parser("list", help="show the comics on a Vita connected over USB")
    list_.add_argument("--card", type=Path, help=card_help)

    remove = commands.add_parser("remove", help="delete a comic from a Vita connected over USB")
    remove.add_argument("name", help="the comic's name, as `komawari list` shows it")
    remove.add_argument("--card", type=Path, help=card_help)

    commands.add_parser("artwork", help=f"redraw the reader's pictures for the Vita's home screen into {ARTWORK_DIR}/")
    commands.add_parser("manual", help=f"rebuild {MANUAL_PAGE}, the manual, from {MANUAL_SOURCE}")
    return parser


def _add_direction_options(command: argparse.ArgumentParser) -> None:
    """Adds --ltr and --rtl. Without either, args.right_to_left is None: "find out yourself"."""
    direction = command.add_mutually_exclusive_group()
    direction.add_argument(
        "--ltr",
        dest="right_to_left",
        action="store_const",
        const=False,
        default=None,
        help="the comic reads left to right (western comics)",
    )
    direction.add_argument(
        "--rtl",
        dest="right_to_left",
        action="store_const",
        const=True,
        help="the comic reads right to left (manga); this is also what is assumed when nothing says otherwise",
    )


# --- Making comics for the Vita ---------------------------------------------


def _reading_direction(archive: Path, asked: bool | None) -> tuple[bool, str]:
    """Decides whether a comic reads right to left, and says where that came from.

    What was asked for on the command line wins. Otherwise the archive is asked
    (see stated_reading_direction). Otherwise it is right to left, as in manga.
    """
    if asked is not None:
        return asked, "as asked"
    stated = stated_reading_direction(archive)
    if stated is not None:
        return stated, "from its ComicInfo.xml"
    return True, "assumed, --ltr changes it"


def _direction_words(right_to_left: bool) -> str:
    return "right to left" if right_to_left else "left to right"


def prepare_comics(archives: list[Path], asked_direction: bool | None, force: bool) -> None:
    """Prepares the given archives, or every archive in comics/ when none is given."""
    _require_kumiko()
    archives = archives or list_archives(COMICS_DIR)
    if not archives:
        sys.exit(f"No comic archives in {COMICS_DIR}/.")
    prepared = [archive.name for archive in archives if _prepare_comic(archive, asked_direction, force)]
    print(f"Done: {len(prepared)} prepared, {len(archives) - len(prepared)} skipped.")


def _prepare_comic(archive: Path, asked_direction: bool | None, force: bool) -> bool:
    """Prepares one comic. Returns False if it was skipped because it is already prepared.

    The work has three stages: extract the pages, find the panels, write the
    Vita-sized copy. A stage is skipped when its result is already there, so
    deleting for_vita/NAME and running this again redoes only the last one.
    """
    if not archive.is_file() or archive.name.startswith("."):
        sys.exit(f"{archive} is not a comic archive.")
    book_dir = BOOKS_DIR / archive.stem
    target_dir = FOR_VITA_DIR / archive.stem
    if force:
        shutil.rmtree(book_dir, ignore_errors=True)
        shutil.rmtree(target_dir, ignore_errors=True)

    try:
        details = read_details(archive)
    except ValueError as error:
        sys.exit(str(error))

    if (target_dir / "panels.json").exists():
        print(f"Skipping {archive.name}: {target_dir}/ already exists.")
        if refresh_details(target_dir, details, ARTWORK_FONTS_DIR):
            print(f"  Brought its title, subtitle and author up to date ({details_file(archive)}).")
        _remove_full_size_pages(book_dir)
        return False

    right_to_left, why = _reading_direction(archive, asked_direction)
    direction = "rtl" if right_to_left else "ltr"
    print(f"Preparing {archive.name}, read {_direction_words(right_to_left)} ({why})")
    pages_dir = _extract_pages(archive, book_dir)
    panels_file = book_dir / "panels.json"
    if panels_file.exists() and load_book(panels_file).reading_direction == direction:
        print(f"  Panels already found ({panels_file}); not looking for them again.")
    else:
        _find_panels(archive, pages_dir, panels_file, right_to_left)

    print(f"  Resizing the pages to fit {MAX_WIDTH} x {MAX_HEIGHT}")
    copy = make_vita_copy(book_dir, target_dir, details, ARTWORK_FONTS_DIR)
    print(f"  Wrote {target_dir}/: {len(copy.book.pages)} pages, cover.jpg and panels.json")
    if copy.pages_made_smaller:
        print(f"  {copy.pages_made_smaller} wide or colour pages were made smaller, so that the Vita loads them faster")
    _remove_full_size_pages(book_dir)
    return True


def _extract_pages(archive: Path, book_dir: Path) -> Path:
    """Extracts the pages that are not yet in book_dir/pages and returns that folder."""
    pages_dir = book_dir / "pages"
    pages_dir.mkdir(parents=True, exist_ok=True)
    missing = [name for name in list_images(archive) if not (pages_dir / Path(name).name).exists()]
    if missing:
        print(f"  Extracting {len(missing)} pages to {pages_dir}/")
        extract(archive, missing, pages_dir)
    return pages_dir


def _find_panels(archive: Path, pages_dir: Path, panels_file: Path, right_to_left: bool) -> None:
    """Runs Kumiko on the extracted pages and writes the comic's full-size panels.json."""
    names = list_images(archive)
    print(f"  Running kumiko on {len(names)} pages")
    found = run_kumiko([pages_dir / Path(name).name for name in names], KUMIKO_SCRIPT, right_to_left)
    book = Book(
        title=archive.stem,
        subtitle="",
        author="",
        reading_direction="rtl" if right_to_left else "ltr",
        detector=found.detector,
        pages=[without_false_panels(page) for page in found.pages],
    )
    save_book(book, panels_file)
    panel_count = sum(len(page.panels) for page in book.pages)
    print(f"  Wrote {panels_file}: {len(book.pages)} pages, {panel_count} panels")


def _remove_full_size_pages(book_dir: Path) -> None:
    """Deletes the extracted full-size pages of a finished comic; they are large
    and can be extracted from the archive again."""
    pages_dir = book_dir / "pages"
    if pages_dir.is_dir():
        size = folder_size(pages_dir)
        shutil.rmtree(pages_dir)
        print(f"  Removed the full-size pages in {pages_dir}/ ({_megabytes(size)}).")


def test_comic(archive: Path | None, asked_direction: bool | None) -> None:
    """Checks the panel detection on a few random pages and writes an HTML page showing the result."""
    _require_kumiko()
    archive = archive or newest_archive(COMICS_DIR)
    if archive is None:
        sys.exit(f"No comic archives in {COMICS_DIR}/.")
    right_to_left, why = _reading_direction(archive, asked_direction)
    print(f"Checking {TEST_PAGE_COUNT} random pages of {archive.name}, read {_direction_words(right_to_left)} ({why})")
    report_file = spot_check(archive, TEST_DIR, KUMIKO_SCRIPT, TEST_PAGE_COUNT, right_to_left)
    print(f"Wrote {report_file}. Open it in a browser to see the panels and their reading order.")


def _require_kumiko() -> None:
    if not KUMIKO_SCRIPT.exists():
        sys.exit("Kumiko is missing. Run `git submodule update --init` first.")


# --- The Vita -----------------------------------------------------------------


def install_reader(card_argument: Path | None) -> None:
    """Copies the reader's package to the Vita, ready to be installed from VitaShell.

    The package built on this computer is used if there is one. Otherwise the
    newest released package is downloaded.
    """
    card = _require_card(card_argument)
    if READER_PACKAGE.is_file():
        package = READER_PACKAGE
        print(f"Using the reader built on this computer: {package}")
    else:
        package = DOWNLOADED_PACKAGE
        print(f"Downloading the reader from {LATEST_PACKAGE_URL}")
        if not download_latest_package(package):
            sys.exit("The download did not work. Check the internet connection and try again.")
    copied_to = copy_reader(package, card)
    print(f"Copied the reader to {copied_to}.")
    print("To install it: leave USB mode, then in VitaShell go to ux0:data/ and press Cross on komawari.vpk.")


def transfer_comics(card_argument: Path | None) -> None:
    """Copies every prepared comic that is new, or prepared again since it was copied, to the Vita."""
    card = _require_card(card_argument)
    comic_dirs = sorted(path for path in FOR_VITA_DIR.glob("*") if (path / "panels.json").is_file())
    if not comic_dirs:
        sys.exit(f"No prepared comics in {FOR_VITA_DIR}/. Run `komawari prepare` first.")

    copied = 0
    for comic_dir in comic_dirs:
        name = comic_dir.name
        if is_same_on_card(comic_dir, card):
            print(f"Skipping {name}: already on the Vita.")
            continue
        size, free = folder_size(comic_dir), free_space(card)
        if not is_on_card(card, name) and size > free:
            print(f"Skipping {name}: it needs {_megabytes(size)}, the card has {_megabytes(free)} free.")
            continue
        if is_on_card(card, name):
            print(f"Updating {name}: it was prepared again since it was copied.")
        else:
            print(f"Copying {name} ({_megabytes(size)}) to {card / COMICS_FOLDER}/")
        copy_comic(comic_dir, card)
        copied += 1
    print(f"Done: {copied} copied, {len(comic_dirs) - copied} skipped.")
    print("Eject the drive on the computer before leaving USB mode on the Vita.")


def remove_from_vita(name: str, card_argument: Path | None) -> None:
    """Deletes one comic from the Vita, after asking. The prepared copy on the computer stays."""
    card = _require_card(card_argument)
    on_card = {comic.name: comic for comic in comics_on_card(card)}
    if name not in on_card:
        names = ", ".join(on_card) or "none"
        sys.exit(f"There is no comic called {name!r} on the Vita. On the Vita: {names}.")
    answer = input(f"Delete {name} ({_megabytes(on_card[name].size_bytes)}) from the Vita? Type y to confirm: ")
    if answer.strip().lower() != "y":
        print("Nothing was deleted.")
        return
    remove_comic(card, name)
    print(f"Deleted {name} from the Vita. Free space on the card: {_megabytes(free_space(card))}")


def list_comics(card_argument: Path | None) -> None:
    card = _require_card(card_argument)
    comics = comics_on_card(card)
    print(f"Comics on the Vita ({card / COMICS_FOLDER}/):")
    for comic in comics:
        print(f"  {comic.name:<32} {comic.page_count:>4} pages  {_megabytes(comic.size_bytes):>9}")
    if not comics:
        print("  none")
    print(f"Free space on the card: {_megabytes(free_space(card))}")


def _require_card(card_argument: Path | None) -> Path:
    card = card_argument or find_card()
    if card is None or not card.is_dir():
        sys.exit(
            "No Vita memory card found.\n"
            'On the Vita: open VitaShell, press START, set "SELECT button" to "USB", '
            "connect the cable and press SELECT.\n"
            "If the card is connected but not found, name its top folder with --card."
        )
    return card


def _megabytes(size_bytes: int) -> str:
    return f"{size_bytes / 1_000_000:,.0f} MB"


# --- The reader's pictures --------------------------------------------------


def draw_artwork() -> None:
    for path in write_artwork(ARTWORK_FONTS_DIR, ARTWORK_DIR):
        print(f"Wrote {path}")
    ARTWORK_PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    preview(ARTWORK_DIR).save(ARTWORK_PREVIEW)
    print(f"Wrote {ARTWORK_PREVIEW}: roughly how the Vita will show them")


# --- The manual -----------------------------------------------------------------


def write_manual() -> None:
    MANUAL_PAGE.write_text(build_manual(MANUAL_SOURCE), encoding="utf-8")
    print(f"Wrote {MANUAL_PAGE} from {MANUAL_SOURCE}. Open it in a browser.")
