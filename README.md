# Komawari

Komawari is a comic reader for the PS Vita. A whole page is too small to read on the Vita's screen, so Komawari shows one panel at a time and glides to the next.

You prepare your comics on a computer, copy them to the Vita and read them there.

## What you need

- A PS Vita that runs homebrew apps, with VitaShell on it, and its USB cable.
- A Mac or a Linux computer.
- Your comics as CBZ or CBR files.

## 1. Set up the computer

You do this once. Open the Terminal and paste each block.

On Fedora or Nobara Linux only (not tried yet):

```
sudo dnf install git bsdtar
```

Install uv, the tool that runs Komawari:

```
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Close the Terminal and open it again. Then download Komawari:

```
git clone --recurse-submodules https://github.com/kwap/komawari.git
cd komawari
uv sync
```

On a Mac, this may offer to install Apple's developer tools. Say yes, wait until it is done and paste the block again.

There is now a `komawari` folder in your home folder. Every command below is run inside it. In a new Terminal window, go there first:

```
cd komawari
```

## 2. Prepare your comics

1. Copy your comic files into the `comics` folder inside the `komawari` folder.
2. In the Terminal, run:

   ```
   uv run komawari prepare
   ```

This takes a few minutes per comic. Comics that are already prepared are skipped, so run it again whenever you add new ones.

Komawari expects manga, read right to left. For a comic read left to right, run this instead, with the name of your file:

```
uv run komawari prepare "comics/My Comic.cbz" --ltr --force
```

Under each cover the reader shows a title, a subtitle and an author. To set them, put a text file next to the comic, with the same name and the ending `.txt`. For `My Comic.cbz` that is `My Comic.txt`:

```
title: My Comic
subtitle: Part 1
author: Some Body
```

Then run `uv run komawari prepare` again; this takes a moment. Without the file, the title is the name of the comic's file.

## 3. Copy to the Vita

These steps have not been tried with a real Vita yet.

1. On the Vita, open VitaShell, press START and set "SELECT button" to "USB".
2. Connect the Vita to the computer with the cable and press SELECT.
3. On the computer, run:

   ```
   uv run komawari install
   uv run komawari transfer
   ```
4. Eject the Vita's drive on the computer, then leave USB mode on the Vita.
5. In VitaShell, go to `ux0:data/`, press Cross on `komawari.vpk` and answer yes.

Komawari is now on the Vita's home screen.

To add comics later, repeat part 2 and steps 1 to 4. Step 5 is only needed once.

## Reading

| | Buttons | Touch screen |
|---|---|---|
| Choose a comic | D-pad left and right | Slide the covers, or tap one |
| Open it | Cross | Tap the cover in the middle |
| Next panel | D-pad right, R or Cross | |
| Previous panel | D-pad left or L | |
| Whole page and back | D-pad up or Triangle | |
| Look closer | | Spread two fingers |
| Move the picture | | Drag it with one finger |
| Put the picture back | | Tap it |
| Back to your comics | Circle | |
| Leave | PS button | |

With only one comic on the Vita, it opens straight away. With three or more, the covers go round: after the last comic comes the first again.

Komawari remembers your place. Each comic opens where you stopped reading it, and the shelf opens on the comic you read last.

A tall page that is shown whole, such as a cover, is small on the Vita's wide screen. Hold the Vita upright, like a book, and the page turns with it and fills the screen.

## Commands

Run them in the `komawari` folder.

| Command | What it does |
|---|---|
| `uv run komawari prepare` | Prepares every comic in `comics`. |
| `uv run komawari test` | Shows how the panels were found on 5 random pages of your newest comic. Open `test/report.html` to look. |
| `uv run komawari install` | Downloads Komawari for the Vita and copies it there. |
| `uv run komawari transfer` | Copies new comics to the Vita. |
| `uv run komawari list` | Lists the comics on the Vita and the free space. |
| `uv run komawari remove NAME` | Deletes a comic from the Vita. |

| Option | Meaning |
|---|---|
| `--ltr` | For `prepare` and `test`: the comic reads left to right. |
| `--rtl` | For `prepare` and `test`: the comic reads right to left. |
| `--force` | For `prepare`: prepare a comic again. |
| `--card FOLDER` | For the four Vita commands: where the Vita's drive is, if it is not found. |
| `--help` | Shows everything a command can do. |

## If something goes wrong

- **"No Vita memory card found."** Check that the Vita is in USB mode. If you can see its drive on the computer, name it: `uv run komawari transfer --card /Volumes/NAME` on a Mac, `--card /run/media/YOU/NAME` on Linux.
- **Copying does not work at all.** Copy by hand: the folders in `for_vita` go into `data/komawari` on the Vita's drive. `komawari.vpk`, from the [download page](https://github.com/kwap/komawari/releases/latest), goes into `data`.
- **A red screen, then Komawari closes.** It found no comics on the Vita. `uv run komawari list` shows what is there.
- **Panels come in the wrong order.** The comic was prepared for the wrong direction. Prepare it again with `--ltr --force` or `--rtl --force`, then run `transfer`.
- **A panel is cut badly.** The panels are found by a program, and some come out wrong. Press Triangle to see the whole page.

## Licence

Komawari is free to use, change and share under the MIT licence; see `LICENSE`. It finds the panels with [Kumiko](https://github.com/njean42/kumiko), a separate program with its own licence. `docs/licences.md` lists everything Komawari uses.
