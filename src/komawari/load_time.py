"""Estimates how long the Vita reader needs to load the picture of one page.

The numbers were measured on a Vita on 2026-10-10 and 2026-10-11 (docs/notes.md, "Slow loading").
Loading has a part that grows with the number of pixels and a part that grows
with the size of the file. A colour picture costs far more per pixel than a grey one.
"""

# Milliseconds per million pixels: decoding, plus handing the picture to the screen.
GREY_MS_PER_MEGAPIXEL = 77 + 46
COLOUR_MS_PER_MEGAPIXEL = 210 + 195
# Milliseconds per kilobyte of file, for decoding.
DECODE_MS_PER_KILOBYTE = 0.24
# Milliseconds per kilobyte of file, for reading it from the memory card: about
# 8 MB per second, with the large reads the reader makes since 2026-10-11.
# With reads of 1 KB it was 0.35.
READ_MS_PER_KILOBYTE = 0.12


def load_seconds(pixels: int, file_bytes: int, in_colour: bool) -> float:
    """The time the Vita takes to read, decode and show a JPEG picture of this size."""
    per_megapixel = COLOUR_MS_PER_MEGAPIXEL if in_colour else GREY_MS_PER_MEGAPIXEL
    per_kilobyte = DECODE_MS_PER_KILOBYTE + READ_MS_PER_KILOBYTE
    milliseconds = pixels / 1_000_000 * per_megapixel + file_bytes / 1024 * per_kilobyte
    return milliseconds / 1000
