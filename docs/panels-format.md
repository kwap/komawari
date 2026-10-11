# The panels file (`panels.json`), format 1

One file per comic. It lists the pages in reading order and, for each page, the panels in reading order. It sits next to a `pages/` folder that holds the page images.

`komawari prepare` writes two of them for a comic:

- `books/NAME/panels.json`, with the full-size pages in `books/NAME/pages/`.
- `for_vita/NAME/panels.json`, with the smaller pages in `for_vita/NAME/pages/`. This folder is what the reader loads, and what is copied to the Vita.

A folder in `for_vita/` also holds two pictures that the reader shows where a comic is chosen:

- `cover.jpg`, 544 pixels high. It is made from the first page that is taller than wide.
- `title.png`, 720 x 100 pixels, see-through around the letters: the comic's title, subtitle and author, drawn in the fonts of the reader's artwork. The reader cannot write text, so it shows this picture under the cover. A comic without it has no text there.

## Example

```json
{
  "format": 1,
  "title": "My Comic",
  "subtitle": "Part 1",
  "author": "Some Body",
  "reading_direction": "rtl",
  "detector": "kumiko",
  "pages": [
    {
      "file": "MyComic_v01_001.png",
      "width": 4130,
      "height": 5960,
      "panels": []
    },
    {
      "file": "MyComic_v01_014.png",
      "width": 4130,
      "height": 5960,
      "panels": [
        { "x": 2143, "y": 262, "width": 1700, "height": 1840 },
        { "x": 290, "y": 262, "width": 1820, "height": 1840 }
      ]
    }
  ]
}
```

The numbers in the example are made up.

## Fields

| Field | Meaning |
|---|---|
| `format` | Version of this structure, a whole number. A reader refuses a number it does not know. |
| `title` | Name of the comic. From the text file next to the archive (`NAME.txt`, see `src/komawari/details.py`), else the archive's file name. |
| `subtitle`, `author` | From the same text file; empty when not given. Added on 2026-10-11 without a new format number: a file from before has neither, and the reader does not read them. It shows `title.png` instead. |
| `reading_direction` | `"rtl"` or `"ltr"`. The panels are already in order; the reader can use this for the direction of page turns. |
| `detector` | The tool that produced the panels. Information only. |
| `pages` | The pages, in reading order. |
| `pages[].file` | File name of the page image, inside the `pages/` folder next to this file. |
| `pages[].width`, `pages[].height` | Size of that image in pixels. |
| `pages[].panels` | The panels, in reading order. |
| `panels[].x`, `panels[].y` | Top-left corner of the panel, in pixels from the page's top-left corner. |
| `panels[].width`, `panels[].height` | Size of the panel in pixels. |

## Rules

- All numbers are whole numbers.
- An empty `panels` list means: show the whole page. `komawari prepare` empties the list in three cases:
  - the file name has no page number at its end, or the number 0 (covers and other extras, such as `..._Cover.jpg`, `..._000a.jpg` and `... p000.jpg`);
  - only one panel was detected;
  - the detected panels cover less than half the page (title pages and text pages).
- Rectangles are in the pixels of the image named by `file`, so a reader uses them as they are. `komawari resize` converts them when it makes the smaller images.
- In the Vita copy no image is wider than 4096 or taller than 2560 pixels, and all images are JPEG.
- Rectangles may overlap and may leave parts of the page uncovered.
- A double-page spread stored as one image is one page.
