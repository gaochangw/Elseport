#!/usr/bin/env python3
# $Id$
# Elseport - assemble a local Qt prefix without changing installed packages.
# SPDX-License-Identifier: GPL-3.0-only

import argparse
import json
from pathlib import Path
import subprocess


def link(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.is_symlink() and target.resolve() == source.resolve():
        return
    if target.exists() or target.is_symlink():
        raise RuntimeError(f'Refusing to replace existing path: {target}')
    target.symlink_to(source.resolve(), target_is_directory=source.is_dir())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('prebuild/qt6'))
    args = parser.parse_args()
    output = args.output.resolve()
    versions = {}
    for module in ('qtbase', 'qttools', 'qtscxml', 'qt5compat', 'qtsvg'):
        prefix = Path(subprocess.check_output(['brew', '--prefix', module], text=True).strip()).resolve()
        if not prefix.is_dir():
            parser.error(f'Qt module is not installed: {module}')
        versions[module] = str(prefix)
        for directory in ('include', 'lib', 'bin', 'libexec'):
            source = prefix / directory
            if source.is_dir():
                for child in source.iterdir():
                    target = output / directory / child.name
                    if target.is_symlink():
                        continue
                    link(child, target)
        plugins = prefix / 'share/qt/plugins'
        if plugins.is_dir():
            for category in plugins.iterdir():
                if category.is_dir():
                    for child in category.iterdir():
                        target = output / 'plugins' / category.name / child.name
                        if not target.exists():
                            link(child, target)
    (output / 'module-prefixes.json').write_text(json.dumps(versions, indent=2) + '\n')
    print(output)


if __name__ == '__main__':
    main()
