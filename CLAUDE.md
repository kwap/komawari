# Panel-by-panel comic reader for the PS Vita

A PC tool (Python, this repository's `src/komawari`) finds the panels on comic pages. The reader (C with SDL2, in `reader/`) steps through them; the same code runs in a desktop window and on the Vita.

Start every session by reading `docs/notes.md`. It holds the current state and what is next.

## Commands

```
git submodule update --init                 # fetch Kumiko and cJSON into third_party/ (once per clone)
uv sync                                     # create or update .venv from uv.lock
uv run pyright                              # type check; must report 0 errors

uv run komawari prepare                     # every archive in comics/ -> for_vita/NAME/; skips what is already done
                                            #   reading direction: the archive's ComicInfo.xml, else right to left;
                                            #   --ltr or --rtl to say it yourself; --force to redo
uv run komawari test                        # 5 random pages of the newest comic -> test/report.html
uv run komawari manual                      # README.md -> index.html, the manual and the website

# With a Vita connected over USB (VitaShell USB mode):
uv run komawari install                     # copy the reader to the Vita: build/vita/komawari.vpk if it was
                                            #   built here, else the latest release from GitHub
uv run komawari transfer                    # copy new and changed comics from for_vita/
uv run komawari list                        # the comics on the Vita
uv run komawari remove NAME                 # delete one comic from the Vita (asks first)
```

Run the commands from the repository root.

The reader:

```
cmake --preset desktop && cmake --build --preset desktop   # build/desktop/reader, a 960 x 544 window
cmake --preset vita && cmake --build --preset vita         # build/vita/komawari.vpk; SDK from VITASDK or ~/vitasdk
cmake --build --preset vita --target emulator              # the same, then start it in Vita3K
uv run komawari artwork                                    # redraw the pictures in reader/sce_sys/ (bubble, page, loading screen)
```

## C rules (reader)

- C11. `-Wall -Wextra` must stay free of warnings in our own files.
- Only SDL2 calls for windows, input, files and images, so the code stays the same on the desktop and the Vita. The one exception is listing a folder (`reader/library.c`), which SDL2 cannot do; it uses POSIX `dirent.h`.
- One job per file; each header starts with a comment saying what the file is for. Format with `clang-format` (settings in `.clang-format`).
- Structs for structured data (`Panel`, `Page`, `Book`).

## Python rules

- Type hints on every function. `pyright` runs in strict mode and must pass.
- Dataclasses for structured data (for example `Panel`, `Page`). No loose dictionaries passed around.
- Small modules with one job each.
- Python only on the PC side.
- Everything must work on macOS and on Linux.

## Working rules

- Work in small increments, each one tried before the next.
- Build only what the current step needs. No extra features, options or abstractions for possible future needs.
- When there are real alternatives, list them with tradeoffs and say which one to pick.
- Check library facts against the current repositories and documentation, not from memory.
- Do not build correction or editing tooling in advance.
- Write for readers in plain language and short sentences.

## Repository rules

- The repository is public: https://github.com/kwap/komawari. Nothing in it names a specific comic or describes anyone's own computer. That includes the notes, code comments, examples and commit messages.
- Never commit comic pages or archives. They are copyrighted. `comics/` is git-ignored, and so are all image and archive files anywhere in the repository. Do not weaken those rules.
- Commit `uv.lock`.
- `docs/decisions.md`: every decision with a one-line reason and date.
- `docs/notes.md`: current state, what was done last, what is next, and what a developer needs to know. Update it at the end of every session.
- `docs/licences.md`: the licence of every tool and model used.
- `docs/panels-format.md`: the structure of `panels.json`. Change it together with `src/komawari/book.py` and the reader.
- `README.md`: the user's guide, for someone who wants Komawari on their Vita and is not technical. Keep it short and free of jargon: the steps from nothing to reading, the reader's buttons, the commands. Keep it in step with `src/komawari/cli.py` and `reader/input.c`. What only a developer needs goes in `docs/notes.md`.
- `index.html`: the manual, built from `README.md`. It is also the website, https://kwap.github.io/komawari/, which GitHub serves from the `main` branch. Never edit it by hand. After changing `README.md`, run `uv run komawari manual` and commit the new page with the change.
- `.github/workflows/build.yml` builds `komawari.vpk` on GitHub on every push. Pushing a tag such as `v0.2.0` publishes it as a release, which is where `komawari install` downloads the reader from.
