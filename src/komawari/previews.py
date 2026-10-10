"""Writes small copies of the pages for the report.

A full-size page can be 4000 x 6000 pixels. A browser showing ten of those,
several times each, becomes slow, so the report uses reduced copies.
"""

from pathlib import Path

from PIL import Image

PREVIEW_WIDTH = 1600


def preview_name(page_file_name: str) -> str:
    return f"{Path(page_file_name).stem}.jpg"


def write_previews(page_files: list[Path], folder: Path) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    for page_file in page_files:
        with Image.open(page_file) as image:
            width = min(image.width, PREVIEW_WIDTH)
            height = round(image.height * width / image.width)
            small = image.convert("RGB").resize((width, height), Image.Resampling.LANCZOS)
        small.save(folder / preview_name(page_file.name), quality=85)
