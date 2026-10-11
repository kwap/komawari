# Licences

Komawari itself is under the MIT licence; see `LICENSE` in the top folder. This file lists the licence of every tool and model that Komawari uses or that was evaluated for it.

Checked on 2026-10-09 against the repositories, PyPI and the Hugging Face model pages.

## In use

| Tool | Version | Licence | Source |
|---|---|---|---|
| uv | 0.11.25 | MIT OR Apache-2.0 | https://pypi.org/project/uv/ |
| pyright | 1.1.414 | MIT | https://pypi.org/project/pyright/ |
| Kumiko (git submodule, not copied into this repository) | v1.5 (commit 9d587ae) | AGPL-3.0-or-later | https://github.com/njean42/kumiko/blob/master/LICENSE |
| OpenCV (`opencv-python-headless`) | 4.14.0 | Apache-2.0 | https://pypi.org/project/opencv-python-headless/ |
| Pillow | 12.3.0 | MIT-CMU | https://pypi.org/project/pillow/ |
| numpy | 2.5.3 | BSD-3-Clause and others (see PyPI) | https://pypi.org/project/numpy/ |
| requests | 2.34.2 | Apache-2.0 | https://pypi.org/project/requests/ |
| markdown-it-py (builds the manual; checked 2026-10-10) | 4.2.0 | MIT | https://pypi.org/project/markdown-it-py/ |
| bsdtar (libarchive, system tool) | as installed | BSD-2-Clause (not re-checked) | https://github.com/libarchive/libarchive |
| curl (system tool; downloads the released reader) | as installed | curl licence, which is close to MIT (not re-checked) | https://curl.se/docs/copyright.html |

These are the direct dependencies. `uv.lock` lists everything they pull in.

## Used by the reader

Licences as GitHub reports them for each repository, read on 2026-10-09.

| Tool | Version | Licence | Where |
|---|---|---|---|
| SDL2 | 2.32.8 on the Vita; the system's own on the desktop | Zlib | Vita SDK package `sdl2` |
| SDL2_image | 2.8.12 | Zlib | Vita SDK package `sdl2_image` |
| cJSON | 1.7.19 | MIT | git submodule `third_party/cJSON` |
| vitasdk (GCC 15.2.0 cross compiler, newlib, vita-headers, tools) | 2026.08.1 | Several: vita-headers is MIT; GitHub reports GPL-2.0 for vitasdk's newlib. Not examined further | installed with `vdpm` |
| libwebp 1.6.0, linked in through SDL2_image on the Vita | 1.6.0 | Not checked | Vita SDK package |
| CMake | 3.26 or newer | Not checked | system package |
| Dela Gothic One (font for the name in the artwork and for a comic's title in `title.png`; file in `reader/artwork/`) | from google/fonts, 2026-10-10 | SIL Open Font License 1.1 | https://github.com/syakuzen/DelaGothic |
| Zen Kaku Gothic New Bold (font for the tagline in the artwork and for a comic's subtitle and author in `title.png`; file in `reader/artwork/`) | from google/fonts, 2026-10-10 | SIL Open Font License 1.1 | https://github.com/googlefonts/zen-kakugothic |
| Vita3K (emulator, for testing only) | continuous build of 2026-10-08 | GPL-2.0 | https://vita3k.org |
| `actions/checkout` and `actions/upload-artifact` (the build on GitHub; checked 2026-10-10) | v7 | MIT | https://github.com/actions |

## Used by the manual

The manual (`index.html`) shows its text in two fonts. They are files in `fonts/`, next to their licence texts. Checked on 2026-10-10.

| Font | Version | Licence | Where |
|---|---|---|---|
| Bricolage Grotesque (a subset made with fontTools: Latin letters, weights 500 to 800) | from google/fonts, 2026-10-10 | SIL Open Font License 1.1, no reserved font name | https://github.com/ateliertriay/bricolage |
| IBM Plex Mono Medium (IBM's own WOFF2 file, unchanged) | from IBM/plex, 2026-10-10 | SIL Open Font License 1.1, with the reserved font name "Plex" | https://github.com/IBM/plex |

Because of the reserved name, the IBM Plex file stays as IBM publishes it; a changed copy would need another name.

The blocks of commands take their colours from the Tokyo Night theme, light and dark (MIT; https://github.com/tokyo-night/tokyo-night-vscode-theme, read on 2026-10-11). Only the colour values are used; none of the theme's files are in this repository.

## What a release contains

The released `komawari.vpk` is the reader as built by GitHub. It holds the reader's own code and artwork (MIT) and, linked in from the Vita SDK: SDL2 and SDL2_image, cJSON, the picture libraries that SDL2_image brings along (libpng, libjpeg-turbo, libwebp, zlib) and the SDK's C library (newlib). Kumiko is not part of a release.

## Evaluated, not in use

Magi and the libraries it needed were part of the project while the detectors were compared, and were removed on 2026-10-10.

| Tool | Version | Licence | Source |
|---|---|---|---|
| Magi v3 (model and its code) | revision 49c73a2 | No licence file. See the two statements below. | https://huggingface.co/ragavsachdeva/magiv3 |
| PyTorch | 2.14.1 | BSD-3-Clause and others (see PyPI) | https://pypi.org/project/torch/ |
| transformers | 4.45.2 | Apache-2.0 | https://pypi.org/project/transformers/ |
| einops | 0.8.2 | MIT | https://pypi.org/project/einops/ |
| timm | 1.0.30 | Apache-2.0 | https://pypi.org/project/timm/ |
| shapely | 2.2.0 | BSD-3-Clause | https://pypi.org/project/shapely/ |
| networkx | 3.7 | BSD-3-Clause | https://pypi.org/project/networkx/ |
| matplotlib | 3.11.2 | Matplotlib licence (PSF-based) | https://pypi.org/project/matplotlib/ |
| pytorch-metric-learning | 2.9.0 | MIT | https://pypi.org/project/pytorch-metric-learning/ |
| FiftyOne | 1.22.1 | Apache-2.0 | https://github.com/voxel51/fiftyone |
| MongoDB server (bundled by `fiftyone-db`) | 7.0.28 | SSPL v1 | https://github.com/mongodb/mongo |

## Magi's terms

Magi has no licence file and no licence tag. Its terms are stated in two places, and they differ:

- GitHub readme: "The provided models and datasets are available for academic research purposes only."
- Hugging Face model pages (v1, v2 and v3): "The provided model and datasets are available for unrestricted use in personal, research, non-commercial, and not-for-profit endeavors. For any other usage scenarios, kindly contact me via email".

## Fallback detectors (tried once, not in use)

All three are YOLO models that run through the `ultralytics` library, which is AGPL-3.0.

| Model | Licence tag on Hugging Face |
|---|---|
| mosesb/best-comic-panel-detection | apache-2.0 |
| cedarrapidsboy/inkwell-panel-models | other (not read yet) |
| Oliverdsfdsf/comic-panels-text-detect | apache-2.0 |

## Open points

- Kumiko is AGPL: a program that imports it and is distributed must be released under compatible terms. Running it as a separate command avoids that.
- Kumiko is not copied into this repository or into a release. The repository only points to it as a git submodule, and `git clone --recurse-submodules` fetches it from its own repository.
- The licences of libwebp, CMake and the Vita SDK's newlib were not examined.
- Magi is no longer part of the tool, so its non-commercial terms no longer touch it.
