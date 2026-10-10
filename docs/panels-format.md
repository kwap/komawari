# The panels file (`panels.json`), format 1

One file per comic. It lists the pages in reading order and, for each page, the panels in reading order. It sits next to a `pages/` folder that holds the page images.

`komawari prepare` writes two of them for a comic:

- `books/NAME/panels.json`, with the full-size pages in `books/NAME/pages/`.
- `for_vita/NAME/panels.json`, with the smaller pages in `for_vita/NAME/pages/`. This folder is what the reader loads, and what is copied to the Vita.

A folder in `for_vita/` also holds `cover.jpg`, 544 pixels high, which the reader shows where a comic is chosen. It is made from the first page that is taller than wide.

## Example

```json
{
  "format": 1,
  "title": "My Comic 1",
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
| `title` | Name of the comic, taken from the archive's file name. |
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
  - the file name has no page number at its end (covers and other extras, such as `..._Cover.jpg` and `..._000a.jpg`);
  - only one panel was detected;
  - the detected panels cover less than half the page (title pages and text pages).
- Rectangles are in the pixels of the image named by `file`, so a reader uses them as they are. `komawari resize` converts them when it makes the smaller images.
- In the Vita copy no image is wider than 4096 or taller than 2560 pixels, and all images are JPEG.
- Rectangles may overlap and may leave parts of the page uncovered.
- A double-page spread stored as one image is one page.
