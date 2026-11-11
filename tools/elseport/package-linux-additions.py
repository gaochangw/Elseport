#!/usr/bin/env python3
# $Id$
# Elseport - assemble a Linux-only integration ISO for the macOS preview.
# SPDX-License-Identifier: GPL-3.0-only

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--installer', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('This ISO assembly helper uses macOS hdiutil.')
    if not args.installer.is_file():
        parser.error('Build VBoxLinuxAdditions.run from the matching source first.')
    if args.output.exists():
        parser.error('The output exists; choose a new path.')
    root = Path(__file__).resolve().parents[2]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='linux-additions-') as temporary:
        stage = Path(temporary)
        shutil.copy2(args.installer, stage / 'VBoxLinuxAdditions.run')
        # The unattended Arm64 installer requests the architecture-specific name.
        shutil.copy2(args.installer, stage / 'VBoxLinuxAdditions-arm64.run')
        for name in ('COPYING', 'LICENSE.txt', 'NOTICE'):
            shutil.copy2(root / name, stage / name)
        subprocess.run(['/usr/bin/hdiutil', 'makehybrid', '-iso', '-joliet',
                        '-default-volume-name', 'ELSEPORT_LINUX',
                        '-o', str(args.output.resolve()), str(stage)], check=True)
    print(args.output.resolve())
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
