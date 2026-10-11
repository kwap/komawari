# Notes

Last updated: 2026-10-11.

## Current state

Komawari is public: the code at https://github.com/kwap/komawari and the manual at https://kwap.io/komawari/.

The reader works in a window on the computer, in the Vita3K emulator and on a real Vita: a shelf for choosing a comic by its cover, then panel-by-panel reading with a gliding view.

| Part | Status |
|---|---|
| Panel detector | Kumiko, chosen after a comparison (see "Why Kumiko") |
| `komawari` command | Prepares comics, checks the detection, copies to a Vita, draws the artwork, builds the manual |
| Reader | Shelf and reading screen, confirmed in the emulator. Background loading, touch and the details under the covers (2026-10-11) run on a real Vita |
| Reader on a real Vita | Installed and used with three volumes (2026-10-10). It works. New pages are slow to appear; the loading time is measured (see "Next") |
| `install`, `transfer`, `list`, `remove` | Tried against a stand-in folder shaped like a memory card, not a real one |
| Linux | Only the build of the reader, on GitHub. Nothing else was tried on Linux |
| GitHub | Repository, website, a build of `komawari.vpk` on every push, releases |

## Done last

- Wide and colour pages are at 80% instead of 70% (2026-10-11, owner's choice), and the name on the page behind the bubble sits lower, clear of the Vita's own icons. Both are on the Vita; neither has been looked at there yet.
- Colour pages (2026-10-11): the reader converts colour pictures to 16-bit colour itself, and `prepare` makes colour pages 70% like the wide pictures. Tried on the Vita by the owner at 70%: fast, sharp enough, no complaints.
- Wide pictures are made at 70% (2026-10-11), after a size test on the Vita (see "What preparing a comic gives"). The three volumes were prepared again.
- The shelf shows title, subtitle and author under the chosen cover (2026-10-11). `prepare` draws them into `title.png` from a text file next to the archive (`src/komawari/details.py`, `src/komawari/title_picture.py`). It ran on the Vita during the size test; the owner did not comment on its look. A title picture loads in 15 ms.
- First word from the Vita on background loading and touch (2026-10-11): much better; at a normal reading pace a page turn is practically instant. Colour spreads still load noticeably slowly. The numbers from the log are under "Next", point 1.
- `prepare` makes the slowest pages smaller (2026-10-11): any page the Vita would need more than 3 seconds to load, by the estimate in `src/komawari/load_time.py`. The three volumes were prepared again and the changed files copied to the Vita over FTP. On the Vita those pages load in 3.0 to 3.1 s, as estimated.
- Background loading (2026-10-11): `reader/pages.c` loads pages on a second thread, the page being read first and then the next one. Tried on a Vita: at a reading pace, page turns are instant.
- Touch (2026-10-11): sliding covers and taps on the shelf, a ring of covers with three comics or more, pinch and drag on a page. `README.md` lists the gestures. Tried on a Vita by the owner, who reported no problem with it.
- Loading time measured on a real Vita (see "Next", point 1). The reader now writes it to the log: one line per picture loaded (`reader/picture.c`) and one line per step (`reader/reading.c`). How pages are loaded was not made faster yet.
- First run on a real Vita. The reader's package and three prepared comics were copied over FTP, by hand with `lftp` (see "On the Vita"), the package was installed from VitaShell, and the reader was used. The shelf and the panel-by-panel reading work.
- The manual has the look of the owner's home page, kwap.io: heavy narrow capitals, thick rules and a switch between light and dark (2026-10-10). The look is in `src/komawari/manual.py`.
- Published on GitHub under the MIT licence, with a build of the reader on every push and a first release.
- `komawari install` downloads the reader from the latest release when it was not built on this computer, so the user's guide no longer asks for CMake or the Vita SDK.
- Notes, decisions and licences cleaned for publication: no specific comics, no private computer.
- The user's guide (`README.md`) rewritten short and plain; `komawari manual` builds `index.html` from it.
- Reading direction: from `--ltr` or `--rtl`, else from the archive's `ComicInfo.xml`, else right to left.
- Command line reworked: `prepare` (everything in `comics/`, skipping what is done), `test`, `install`, `transfer`, `list`, `remove`.
- Artwork for the Vita's home screen, drawn by `komawari artwork`.

## Next

Three improvements were asked for after the first use on a real Vita. They are in the owner's order. All three are built and run on the Vita. The owner is content with loading (2026-10-11); what is still open is listed under each point.

### 1. Slow loading

Measured on a real Vita on 2026-10-10, over the first 14 pages of one volume. No change was made yet: the owner chooses first.

What the wait is:

- It happens once per page. A step to another panel of the same page, or to the whole page and back, shows its first frame after 0 to 1 ms. Every frame takes 17 ms, also with a whole spread on screen, so drawing is not the problem.
- Its length follows the picture: the number of pixels, the size of the file, and whether the file is colour or grey.

| Picture | Pixels | File | Read | Decode | To screen | Total |
|---|---|---|---|---|---|---|
| Normal page, grey | 1774 x 2560 | 2.0 MB | 0.72 s | 0.80 s | 0.21 s | 1.72 s |
| Nearly blank page, grey | 1774 x 2560 | 87 KB | 0.06 s | 0.35 s | 0.21 s | 0.61 s |
| Normal page, colour | 1774 x 2560 | 1.8 MB | 0.63 s | 1.39 s | 0.88 s | 2.90 s |
| Nearly blank page, colour | 1774 x 2560 | 69 KB | 0.03 s | 0.95 s | 0.89 s | 1.87 s |
| Cover, colour | 1890 x 2560 | 0.9 MB | 0.32 s | 1.23 s | 0.94 s | 2.48 s |
| Spread, colour | 3561 x 2560 | 2.7 MB | 0.93 s | 2.55 s | 1.78 s | 5.26 s |
| Widest picture, colour | 4042 x 2560 | 3.7 MB | 1.27 s | 3.05 s | 2.01 s | 6.33 s |
| Cover on the shelf | 398 x 544 | 68 KB | 0.03 s | 0.06 s | 0.04 s | 0.13 s |

- Opening the shelf with three covers took 0.45 to 0.6 s; the first picture after the start costs about 0.17 s extra. Reading a `panels.json` of 300 KB took 0.2 s.
- About 96% of the prepared pages are grey files; the colour ones are covers and colour openings. The middle grey page is 1.5 MB, which works out at about 1.45 s. Only two grey pages were measured; the colour figures rest on twelve.
- The three steps, as rules that fit every measured picture within a few percent:
  - Read: 0.35 ms per KB, which is 2.8 MB per second.
  - Decode: grey 77 ms per million pixels plus 0.23 ms per KB; colour 210 ms per million pixels plus 0.24 ms per KB.
  - To screen: grey 46 ms per million pixels; colour 195 ms per million pixels.

Why each step takes that long (from the sources, see "Facts checked"):

- Read: the Vita SDK's C library reads a file 1 KB at a time. The SDK's own measurement for that is 2.64 MB per second. A fix from 2026-09-15 reads 64 KB at a time, measured there at 11 MB per second. The supported SDK release, used for this measurement, is older than the fix.
- Decode: SDL2_image uses its built-in decoder (stb_image) on the Vita.
- To screen: SDL turns every picture without transparency into 16-bit colour (RGB565) on the Vita. For a colour picture it uses its slowest, general converter.

Chosen by the owner on 2026-10-11: faster reading of the file, and loading the next page in the background.

Both were tried on a Vita on 2026-10-11, together with the smaller slow pages. From the log of that try:

- Reading is three times as fast: 0.10 to 0.15 ms per KB, about 8 MB per second, where it was 0.35 ms per KB. `reader/file.c` reads with standard C and a buffer of 256 KB, because the SDK's own fix is only in its `nightly` channel; the supported release (`2026.08`), which the build on GitHub also installs, reads 1 KB at a time. `panels.json` took 0.13 s instead of 0.2 s.
- Turning the page: of 15 page turns at a reading pace, 11 had the picture on screen within 17 ms. The others waited 0.9 to 3.1 s: the first page of the comic, and colour pages turned faster than they load.
- A picture's line in the log now has four times. Read, decode and convert run on the loading thread. "To screen" runs on the main thread and stalls drawing: 50 to 75 ms for a normal page, 115 ms for a spread.
- Whole loads: a grey page 1.0 to 1.2 s (was about 1.45 s), a colour page of normal size 2.3 to 2.7 s (was 2.9 s), a grey spread 2.5 s, and the colour spreads and dust jacket that `prepare` made smaller 3.0 to 3.1 s (were 3.9 to 6.3 s).
- The estimate in `src/komawari/load_time.py` takes reading at the measured 0.12 ms per KB.

Still open, to weigh after the next measurement:

- The reader's own colour conversion, timed on the Vita: 40 ms per million pixels, where SDL's took 184. With it and at 70%, a colour page of normal shape loaded in about 1.0 s (was 2.3 to 2.7 s at full size), a colour spread or jacket in 1.0 to 1.1 s (was 1.5 s), a grey spread in 1.2 s. Of 30 steps, 28 were on screen within 20 ms.
- At 80%, which is what the comics on the Vita now have, nothing is timed yet. By the number of pixels these pages should take about a third longer: roughly 1.3 to 1.6 s.
- The numbers in `src/komawari/load_time.py` for a colour picture still include SDL's slow conversion. With the measured new figure a colour picture is estimated faster. That would take wide colour pictures out of the 3-second rule and make them larger than the size the owner chose, so change the numbers and the rule together. Colour pages of normal size (about 2.6 s by the estimate) and grey spreads (about 2.4 s) stay as they are. A lower limit would make those smaller too, and smaller pictures are less sharp when zoomed.
- Smaller files. JPEG quality 80 with optimised tables is 26% smaller than today's 90 (measured on 30 grey pages); optimised tables alone are 6% smaller with no loss. Comics must be prepared and copied again.
- Smaller pages, a height of 2048: 36% fewer pixels, but 62% of the panels keep the screen's sharpness instead of 83%.
- Colour pictures: convert to 32-bit colour in the reader, for which SDL has a fast path. Not measured. Twice the memory for those pictures.
- libjpeg-turbo: the SDK's package is built without its fast code for the Vita's processor (`-DWITH_SIMD=FALSE`), and SDL2_image would have to be rebuilt to use it. Gain unknown.
- A loading indicator. The marks at the bottom already move at the press, before the picture is there.

### 2. The shelf: details under the cover

Built on 2026-10-11. It ran on the Vita, but the owner has not said how it looks. Still to judge there: the three lines under the cover, their sizes and colours, and how they fade while the covers slide.

- Not built: reading the details from an archive's `ComicInfo.xml`, which can hold series, title, number and writer. `prepare` reads only the reading direction from it.
- The reader still cannot write text. Page numbers or any other text that changes while reading would need a font in the reader (for example SDL2_ttf) or a picture of the digits.

### 3. Touch

Built on 2026-10-11 and used on the Vita by the owner, who zoomed into pages without reporting a problem. Not yet judged one by one:

- The shelf: sliding the covers with a finger, a short swipe, taps on the middle cover and on its neighbours, and going round past the last comic.
- A page: spreading and pinching two fingers, dragging with one, a tap to put the panel back, and a step to another panel from a zoomed picture.
- Numbers that may need tuning, all at the top of their files: how far a finger may wander in a tap and how long a tap may last (`reader/input.c`), how short a swipe still moves a cover (`reader/shelf.c`), the closest zoom and how fast the dimming goes (`reader/shot.c`).
- Not built: turning the page by touch, and a flick that carries the covers further than one.

### Also open

- `install`, `transfer`, `list` and `remove` against a real card. Either put the microSD card straight into the computer, which is also much faster than FTP, or use USB mode with a cable that carries data.
- Texture memory. SDL keeps all pictures in one pool, as large as the Vita's free graphics memory when the first picture is made (the Vita has 128 MB of it in all). In 16-bit colour a normal page takes 9 MB, a spread 18 MB and the widest picture 21 MB. Four pictures are kept at most, so 84 MB at the very worst and about 27 MB in ordinary reading. If a picture loaded ahead does not fit, it is dropped and loaded again when the reader gets there; this path was never seen running.
- The home-screen page was seen on the device: the Vita's own icons covered part of the name, which was then moved down (not yet seen again). The bubble was not judged; the preview (`build/artwork-preview.png`) places the pictures where Vita3K's source says the Vita puts them.
- Try everything on Linux.
- Older candidates: remember the place in each comic; controls that follow the reading direction.

## Open questions

- Page size 4096 x 2560 and JPEG quality 90 were chosen without a Vita. On the device, loading turned out slow (see "Next", point 1). Sharpness and memory were not judged yet.
- On the Vita every page is drawn in 16-bit colour (see "Facts checked"). Whether that shows, in colour pages or in grey gradients, was not judged.
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
- The mouse stands in for a finger in the window: drag with the left button, click for a tap, and the wheel for spreading and pinching two fingers.
- With one comic the reader opens it at once; the shelf shows only with several comics or after going back.
- If no comic is found, or a comic cannot be read, the screen turns red for four seconds.
- On the Vita, SDL writes its log to `ux0:data/SDL_Log.txt`. It adds to the file and never empties it, and it opens and closes the file for every line.
- The log has one line per picture loaded (pixels, file size, time to read, decode and hand to the screen) and one line per step (time until the first frame, slowest frame). A step's line is written when its movement is over, so a step taken within 1.5 s of the one before leaves no line for that one.
- `reader/picture.c` loads a picture in two steps instead of one `IMG_LoadTexture` call. The first reads the file into memory (`reader/file.c`), decodes it and converts it to the renderer's pixel format; it needs no renderer and runs on the loading thread. The second hands the pixels to the renderer on the main thread. SDL's renderer may only be used from the thread that made it.
- The loading thread gets a stack of 512 KB. SDL's own choice on the Vita is 64 KB.
- The Vita build needs `-lpthread` last on the link line, because libwebp uses it; `CMakeLists.txt` does that through `CMAKE_C_STANDARD_LIBRARIES`.
- On the Vita the default heap is 128 MB, so the reader does not set `_newlib_heap_size_user`.
- Testing without a window: a throwaway program can link the reader's files with `-Dmain=reader_main`, push key events from a thread, and switch SDL's `SDL_VIDEO_DUMMY_SAVE_FRAMES` hint on for a moment to save frames, with `SDL_VIDEODRIVER=dummy`.
- The same works for touch: push `SDL_FINGERDOWN`, `SDL_FINGERMOTION` and `SDL_FINGERUP` events with a made-up touch device number. The reader takes them, because it only leaves out devices that SDL knows as touch pads.
- Do not build such a program with `-fsanitize=address` on macOS with Homebrew's SDL2: the library then stops at start-up with an error window. The system's own heap checks work instead (`MallocScribble=1 MallocGuardEdges=1`).

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
- The page's two fonts are files in `fonts/`, with their licence texts. Bricolage Grotesque is a subset made with fontTools (`pyftsubset`: Latin letters, weights 500 to 800). IBM Plex Mono is IBM's own file, unchanged, because its licence reserves the name; see `docs/licences.md`.
- Safari refuses some things for a page opened as a local file that it allows on a website. On the owner's home page a `<link rel="preload">` line for a font made that font fail this way. The manual has no such line. Look at the page in Safari itself, not only in a preview.
- Blocks of commands: a script in the page (`TERMINAL_SCRIPT` in `manual.py`) puts each one in a "terminal" with a copy button and colours the words in the Tokyo Night theme's colours. It reads the lines as shell commands, so another kind of code would be coloured wrongly. The same script is in the Linux Whisper page.
- The page opens dark (`DEFAULT_THEME` in `manual.py`) whatever the system's setting; the switch changes it and the choice is remembered. The mark beside the tagline is sized together with the sentence from the hero's width (`--size` in the stylesheet).

## On the Vita: what the user's guide leaves out

- Each comic is one folder in `ux0:data/komawari/`, holding `panels.json`, `cover.jpg`, `title.png` and `pages/`. The folder names are free; the reader lists every folder that holds a `panels.json`, sorted by name.
- `komawari.vpk` can be installed from any folder on the card. After a new build, copy the new one over and install it the same way; the comics stay.
- Copying over Wi-Fi instead of USB: in VitaShell press START, set "SELECT button" to "FTP" and press SELECT. VitaShell shows an address like `ftp://192.168.1.20:1337`. With an FTP program, copy the comic folders into `ux0:/data/komawari/` and `komawari.vpk` into `ux0:/data/`.
- Copying over FTP was tried on a real Vita (2026-10-10): three volumes, 1.9 GB, at about 1.3 MB per second, so roughly 8 minutes per volume. Every file arrived with the right size.
- VitaShell's FTP server (it answers as FTPVita) shows the card as `/ux0:/`. It has no `NLST` command. `lftp` asks for that when the listing of a folder comes back empty, and then gives up; `set ftp:list-empty-ok yes` avoids it. `curl -T` works as it is.
- The same server lists nothing when `LIST` is given a folder's path, without reporting an error. Step into the folder first (`CWD`), then send `LIST` alone; `curl` and `lftp` do that by themselves, Python's `ftplib` does not. An empty listing can look like a failed copy.
- While the FTP server runs, the Vita's screen turns off after a while but the Vita does not go to sleep, and a copy carries on.
- When copying by hand, send `panels.json` last, as `komawari transfer` does: the reader lists a folder as soon as that file is in it.
- USB mode: on the first try with a real Vita, the computer saw no USB device at all. The cause was not found; a cable that only charges is the usual one.
- VitaShell's START menu has a "USB device" setting. With the card in an SD2Vita adapter it should be `sd2vita`; "Memory Card" offers the Vita's own memory instead. This is from VitaShell's change log and forum threads, not tried.
- A real card used as `ux0:` has the folders `app` and `appmeta` at its top, which is how `komawari` recognises a Vita card. Seen in a listing over FTP.
- START also opens a comic, and L and R also choose one on the shelf.
- Tried on a real Vita: copying over FTP, installing the package from VitaShell, and reading. The rest was written from the Vita SDK's instructions and VitaShell's own texts.

## What preparing a comic gives

Measured on three manga volumes of 409 to 449 pages, scanned at 4130 x 5960 pixels per page.

- Time: about 3 to 8 minutes per volume. Kumiko takes about 4 s per page of that size and runs on half of the processor's cores at once.
- Size for the Vita: 640 to 850 MB per volume. A normal page becomes 1774 x 2560 pixels and a two-page spread 3561 x 2560; the median file is 1.6 MB, the largest 4.6 MB.
- Pages made smaller for loading time, two rules in `src/komawari/vita_copy.py`:
  - A page the Vita would need more than 3 seconds to load is made smaller until it takes about 3. In the three volumes these are 17 pages, all colour: the dust jackets and the colour spreads.
  - A picture that is wider than tall, or in colour, is then made 80% of that width and height. These are 71 of the 1,288 pages (22, 19 and 30 per volume): the 17 above, 25 grey spreads and 29 colour pages of normal shape, of which 7 have panels.
  - The files of the 71 pages shrink from 159 MB at full size to 97 MB. Panels are scaled with their page. Every grey page of normal shape comes out byte for byte as before.
- How the size was chosen: a test comic of six heavy pictures of one volume (the dust jacket, a wide colour picture, three colour spreads, one of them with panels, and a grey spread with panels), each at 100, 80, 70, 50 and 35% of its size, with the number stamped in the corner. A throwaway script made it; it is not in the repository. The owner looked at them on the Vita, zoomed in: 70% is good for all, 50% workable for pictures without text. After reading whole volumes at 70%, the owner asked for 80%.
- Loading times from that test's log, for four colour pictures that took 3.0 to 3.1 s at 100%: 2.0 to 2.1 s at 80%, 1.5 s at 70%, 0.8 s at 50% and 0.4 s at 35%. The time follows the number of pixels closely. The grey spread was not reached in the test; at full size it took 2.5 s, and the estimate for 70% is 1.3 s.
- The new pages and `panels.json` files were copied to the Vita over FTP and every page's size checked there; the test comic was removed. The new sizes have not been looked at on the Vita as part of the real volumes yet.
- Preparing again without looking for panels: delete or move `for_vita/NAME` and run `prepare`. The full-size `panels.json` in `books/NAME` is kept, so only extracting and resizing run, about a minute per volume.
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
- SDL2 on the Vita (2.32.8) reports the front screen as touch device 1, of the kind "direct", and the rear pad as device 2, of the kind "indirect". A finger's place comes as a share of the screen, 0 to 1. SDL also sends every touch again as a mouse click unless the hint `SDL_HINT_TOUCH_MOUSE_EVENTS` is "0".
- The Vita SDK has release channels (`vdpm channels`): `2026.08` is the supported one, with a toolchain of 2026-08-25, and `nightly` is rebuilt continuously. Installing without naming a channel gives the newest supported one. `vdpm status` tells what an installation follows.
- SDL2's Vita renderer offers the pixel formats ABGR8888, ARGB8888, RGB565 and BGR565. `SDL_CreateTextureFromSurface` (SDL 2.32.8) picks RGB565 for a picture without transparency in any other format, which is what a decoded JPEG is. Seen in the log on the device.
- SDL2_image 2.8.12 from the Vita SDK decodes JPEG with stb_image: the library holds `stbi__jpeg` code and no libjpeg calls. A colour JPEG comes out as RGB24, a grey one as INDEX8.
- The Vita SDK's `libjpeg-turbo` package (3.2.0) is built with `-DWITH_SIMD=FALSE`, so without its fast code for ARM.
- The Vita SDK's C library (newlib) gave file reads a 1 KB buffer until the commit "vita: tell stdio what to buffer a file with" of 2026-09-15, which makes it 64 KB. Its comment gives 2.64 MB per second before and about 11 after, measured on hardware. SDL reads files through this library on the Vita. `~/vitasdk/version_info.txt` tells which newlib an installed SDK has. The supported SDK release `2026.08` is from before that commit.
- The Vita SDK's packages include `sdl2` (2.32.8), `sdl2_image`, `libpng`, `libjpeg-turbo`, `libzip` and `zlib`. There is no SDL3 package and no C JSON library.
- Vita programs are built with CMake into a `.vpk` package. Running one needs a Vita with HENkaku, h-encore or similar.
- The pictures for the home screen must be PNG files with a palette of at most 256 colours.
- With the layout style `psmobile`, the Vita puts its start button at x 520 to 800, y 155 to 313 of the 840 x 500 page behind the bubble.

## To check on Linux

- `bsdtar` is installed (on Fedora-based systems it is the package `bsdtar`).
- `uv sync`, `uv run pyright` and `uv run komawari test` work.
- Where a Vita in VitaShell's USB mode shows up, and whether `komawari list` finds it by itself.
- The desktop build of the reader.
