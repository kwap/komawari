# Notes

Last updated: 2026-10-10.

## Current state

Komawari is public: the code at https://github.com/kwap/komawari and the manual at https://kwap.io/komawari/.

The reader works in a window on the computer and in the Vita3K emulator: a shelf for choosing a comic by its cover, then panel-by-panel reading with a gliding view. It has not run on a real Vita.

| Part | Status |
|---|---|
| Panel detector | Kumiko, chosen after a comparison (see "Why Kumiko") |
| `komawari` command | Prepares comics, checks the detection, copies to a Vita, draws the artwork, builds the manual |
| Reader | Shelf and reading screen, confirmed in the emulator |
| Reader on a real Vita | Not tried. The steps are in `README.md` |
| `install`, `transfer`, `list`, `remove` | Tried against a stand-in folder shaped like a memory card, not a real one |
| Linux | Only the build of the reader, on GitHub. Nothing else was tried on Linux |
| GitHub | Repository, website, a build of `komawari.vpk` on every push, releases |

## Done last

- Published on GitHub under the MIT licence, with a build of the reader on every push and a first release.
- `komawari install` downloads the reader from the latest release when it was not built on this computer, so the user's guide no longer asks for CMake or the Vita SDK.
- Notes, decisions and licences cleaned for publication: no specific comics, no private computer.
- The user's guide (`README.md`) rewritten short and plain; `komawari manual` builds `index.html` from it.
- Reading direction: from `--ltr` or `--rtl`, else from the archive's `ComicInfo.xml`, else right to left.
- Command line reworked: `prepare` (everything in `comics/`, skipping what is done), `test`, `install`, `transfer`, `list`, `remove`.
- Artwork for the Vita's home screen, drawn by `komawari artwork`.

## Next

- Try the reader on a real Vita. The home-screen page and bubble can only be judged there; the preview (`build/artwork-preview.png`) places the pictures where Vita3K's source says the Vita puts them.
- Try everything on Linux.
- Candidates after that: remember the place in each comic; a loading indicator; page numbers and titles (needs a font drawn by the reader, for example with SDL2_ttf); controls that follow the reading direction.

## Open questions

- Page size 4096 x 2560 and JPEG quality 90 were chosen without a Vita. Check sharpness, loading time and memory on the device.
- On the Vita, SDL2_image is linked without libjpeg-turbo (only libwebp appears on the link line), so it probably decodes JPEG with its built-in decoder, which is slower. Measure page loading time on a real Vita.
- Spreads: on a two-page spread Kumiko reads row by row across both pages, where the right page should be read completely first.
- A comic that opens with its whole dust jacket shows that first, then the front cover. Blank pages are still shown.
- Slanted panel borders give overlapping or short rectangles; the dimmed surroundings soften this.
- No correction step exists. Kumiko's errors are in `panels.json` as they are.
- The rule for covers and extras assumes page files whose names end in a page number. A comic named differently would have every page shown whole.
- The reader ignores `reading_direction`: Right is always "next". A left-to-right comic has its panels in the right order, but nothing else changes.
- Reading the direction from `ComicInfo.xml` was only tried on small archives made for the test.

## Setting up for development

```
git clone --recurse-submodules https://github.com/kwap/komawari.git
cd komawari
uv sync
uv run pyright     # must report 0 errors
```

- `bsdtar` must be installed. macOS has it; on Fedora-based Linux it is the package `bsdtar`.
- Comic archives go in `comics/`, which is empty after a clone.

## Building the reader

```
cmake --preset desktop && cmake --build --preset desktop   # writes build/desktop/reader
build/desktop/reader                 # shows the comics in for_vita/
build/desktop/reader SOME/FOLDER     # or the comics in another folder

cmake --preset vita && cmake --build --preset vita    # writes build/vita/komawari.vpk
cmake --build --preset vita --target emulator         # then starts it in Vita3K
```

The desktop build needs CMake, a C compiler and the development files of SDL2 and SDL2_image. The code includes `<SDL.h>`, not `<SDL2/SDL.h>`.

The Vita build needs the Vita SDK with its `sdl2` and `sdl2_image` packages:

```
git clone https://github.com/vitasdk/vdpm
cd vdpm
./bootstrap-vitasdk.sh --install-dir ~/vitasdk
VITASDK=~/vitasdk ~/vitasdk/bin/vdpm install sdl2 sdl2_image
```

- `vdpm install` asks before it installs. In a script, set `VDPM_NONINTERACTIVE=1` so that it does not.
- The build finds the SDK through the `VITASDK` variable, or else in `~/vitasdk`. The SDK's own default, `/usr/local/vitasdk`, needs administrator rights.
- Keys in the window: Left and Right, Space or Enter, Up for the whole page, Backspace to go back to the shelf, Escape to quit.
- With one comic the reader opens it at once; the shelf shows only with several comics or after going back.
- If no comic is found, or a comic cannot be read, the screen turns red for four seconds.
- On the Vita, SDL writes its log to `ux0:data/SDL_Log.txt`.
- The Vita build needs `-lpthread` last on the link line, because libwebp uses it; `CMakeLists.txt` does that through `CMAKE_C_STANDARD_LIBRARIES`.
- On the Vita the default heap is 128 MB, so the reader does not set `_newlib_heap_size_user`.
- Testing without a window: a throwaway program can link the reader's files with `-Dmain=reader_main`, push key events from a thread, and switch SDL's `SDL_VIDEO_DUMMY_SAVE_FRAMES` hint on for a moment to save frames, with `SDL_VIDEODRIVER=dummy`.

The emulator:

- The `emulator` target exists when Vita3K is found (on macOS, in the Applications folder). It first copies `for_vita/` to the emulator's memory card, then installs and starts the reader.
- With Vita3K's own key settings: arrow keys for the D-pad, X for Cross, C for Circle, V for Triangle, Q and E for L and R, Enter for START.
- Keyboard keys only become Vita buttons in Vita3K's full interface, not in its plain window (`--frontend sdl`), which is why the target starts the full interface.
- If everything in Vita3K looks blocky, look at its Settings, Graphics, Internal Resolution Upscaling. It belongs at 1x.

## GitHub: the build, releases and the website

- `.github/workflows/build.yml` builds `komawari.vpk` on every push and pull request, with the Vita SDK commands above, and keeps the package as a download of that run.
- Pushing a tag that starts with `v` also publishes the package as a release: `git tag v0.2.0 && git push origin v0.2.0`.
- `komawari install` uses `build/vita/komawari.vpk` when the reader was built on this computer. Otherwise it downloads `https://github.com/kwap/komawari/releases/latest/download/komawari.vpk` with `curl`.
- The website is `index.html`, served by GitHub Pages from the top folder of the `main` branch. The empty file `.nojekyll` tells GitHub to serve the files as they are.
- Its address is https://kwap.io/komawari/. The domain `kwap.io` is set on the owner's personal site project (`kwap/kwap.github.io`), and GitHub then serves the page of every other project of the account under it. Nothing in this repository sets the address. The older address, `kwap.github.io/komawari`, is sent on to the new one.

## On the Vita: what the user's guide leaves out

- Each comic is one folder in `ux0:data/komawari/`, holding `panels.json`, `cover.jpg` and `pages/`. The folder names are free; the reader lists every folder that holds a `panels.json`, sorted by name.
- `komawari.vpk` can be installed from any folder on the card. After a new build, copy the new one over and install it the same way; the comics stay.
- Copying over Wi-Fi instead of USB: in VitaShell press START, set "SELECT button" to "FTP" and press SELECT. VitaShell shows an address like `ftp://192.168.1.20:1337`. With an FTP program, copy the comic folders into `ux0:/data/komawari/` and `komawari.vpk` into `ux0:/data/`.
- START also opens a comic, and L and R also choose one on the shelf.
- All of this was written from the Vita SDK's instructions and VitaShell's own texts, not tried on a real Vita.

## What preparing a comic gives

Measured on three manga volumes of 409 to 449 pages, scanned at 4130 x 5960 pixels per page.

- Time: about 3 to 8 minutes per volume. Kumiko takes about 4 s per page of that size and runs on half of the processor's cores at once.
- Size for the Vita: 640 to 850 MB per volume. A normal page becomes 1774 x 2560 pixels and a two-page spread 3561 x 2560; the median file is 1.6 MB, the largest 4.6 MB.
- Panels: 2,100 to 2,300 in each of the two volumes counted. Most pages have 5 to 7.
- Shown whole: 27 to 44 pages per volume (covers, extras, title and text pages, pages with a single panel).
- Sharpness, estimated from the panel sizes of one volume: at a page height of 2560, 83% of the panels have at least the screen's 960 x 544 pixels when fitted to it, and the rest are enlarged by a median of 1.12. At 2048 it is 62%, at 3072 it is 95%.
- Kumiko crashes on an occasional page (`ValueError: list.remove(x): x not in list` in its `group_small_panels`), whatever the small-panel setting. `kumiko_runner` gives such a page no panels, so it is shown whole, and lists the failures at the end of the run.
- None of the volumes was checked page by page.

## Why Kumiko: the detector comparison (2026-10-09)

Compared on 22 pages of one black-and-white manga volume: 10 picked at random, and 12 picked for being hard (slanted panel borders, art and sound effects that cross borders).

- Kumiko (rules on top of OpenCV's contour finding) and Magi v3 (a model trained on manga) found the same number of panels in the same order on all 10 random pages. On 7 of them their rectangles nearly coincide.
- Both go wrong on the same kinds of page: slanted borders, and panels that share a border line without a gap. Where the two agree, they can still both be wrong.
- On the 12 hard pages, judged by eye: Kumiko was wrong on 5 and Magi on 4, three of them the same pages.
- Magi tends to cut a page into more pieces. Kumiko keeps sound effects that cross a panel border, where Magi tends to cut them off.
- Kumiko's rough error rate: a wrong panel count on about 1 in 10 ordinary pages and on 5 of the 12 hard ones. That is far from the target of 1%, and accepted for now.
- Speed on a recent laptop, on pages of 4130 x 5960: Kumiko about 4 s per page on the processor alone (0.9 s on 1600-pixel copies, with the same panel counts). Magi v3 about 4.2 s per page on the graphics chip after 6 s of loading, and roughly 25 s per page without it.
- Tuning: Kumiko has two options (`--min-panel-size-ratio` and `--no-panel-expansion`) and is about 1,200 lines of rule-based Python with a `--debug` page that shows each step. Magi v3 has no panel settings, gives no confidence per panel, and its training code and images are not public.
- Licence: Kumiko is AGPL-3.0 and is run as a separate program. Magi's terms are research-only or non-commercial, depending on where they are read. See `docs/licences.md`.
- Two YOLO models were tried once, outside the project: `mosesb/best-comic-panel-detection` (fast; merges whole rows on slanted pages) and `cedarrapidsboy/inkwell-panel-models` (split two panels the others merged; overlapping rectangles on slanted pages). Neither gives a reading order. A third, `Oliverdsfdsf/comic-panels-text-detect`, did not load the way its page says. These are the kind that could be retrained on own labelled pages.

Kumiko was chosen: an open licence, adjustable, and it keeps sound effects that cross borders.

## Facts checked

### Kumiko v1.5 (commit 9d587ae, December 2024)

- Not an installable package: no `pyproject.toml`, and its modules import each other as top-level `lib`. It needs a clone of the repository.
- Command: `kumiko -i <file or folder> --rtl -o out.json`.
- Output is a JSON list with one object per page: `filename`, `size` as [width, height], `numbering`, `gutters`, `license`, `panels`, `processing_time`.
- Each panel is [x, y, width, height] in pixels from the top-left corner. Panels are in reading order.
- As a library: `Kumiko({'rtl': True})`, then `parse_image(path)`, then `get_infos()`.
- Other options: `--min-panel-size-ratio`, `--no-panel-expansion`, `--debug`.
- Needs only OpenCV, numpy and requests.

### Magi v3 (`ragavsachdeva/magiv3`, tried and removed)

- Loaded through `transformers` with `trust_remote_code=True`, which downloads and runs the authors' Python code, so the revision should be fixed. It worked with `transformers` 4.45.2 and failed on 5.x.
- `model.predict_detections_and_associations(images, processor)` returns, per page, `panels` as [x1, y1, x2, y2] in pixels, plus `texts`, `characters` and `tails`.
- It does not sort the panels; they come in the order the model wrote them.
- The weights are a 1.7 GB download, and every page is resized to 768 x 768 first.

### The Vita

- SDL2's Vita renderer limits textures to 4096 x 4096.
- The Vita SDK's packages include `sdl2` (2.32.8), `sdl2_image`, `libpng`, `libjpeg-turbo`, `libzip` and `zlib`. There is no SDL3 package and no C JSON library.
- Vita programs are built with CMake into a `.vpk` package. Running one needs a Vita with HENkaku, h-encore or similar.
- The pictures for the home screen must be PNG files with a palette of at most 256 colours.
- With the layout style `psmobile`, the Vita puts its start button at x 520 to 800, y 155 to 313 of the 840 x 500 page behind the bubble.

## To check on Linux

- `bsdtar` is installed (on Fedora-based systems it is the package `bsdtar`).
- `uv sync`, `uv run pyright` and `uv run komawari test` work.
- Where a Vita in VitaShell's USB mode shows up, and whether `komawari list` finds it by itself.
- The desktop build of the reader.
