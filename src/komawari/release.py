"""Downloads the reader's package (komawari.vpk) as published on GitHub.

GitHub builds the package from this repository (.github/workflows/build.yml)
and publishes it as a release, so that nobody has to install the Vita SDK just
to read comics.
"""

import subprocess
from pathlib import Path

# GitHub sends this address on to the package of the newest release.
LATEST_PACKAGE_URL = "https://github.com/kwap/komawari/releases/latest/download/komawari.vpk"


def download_latest_package(target: Path) -> bool:
    """Downloads the newest released package to target. Returns False if that did not work.

    The download is done by the curl command, which macOS and Linux have and
    which prints the reason itself when it fails.
    """
    target.parent.mkdir(parents=True, exist_ok=True)
    download = subprocess.run(
        ["curl", "--fail", "--location", "--silent", "--show-error", "--output", str(target), LATEST_PACKAGE_URL]
    )
    return download.returncode == 0
