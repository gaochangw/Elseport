#!/usr/bin/env python3
# $Id$
# Elseport - archive an exact public source revision and its pinned kBuild tree.
# SPDX-License-Identifier: GPL-3.0-only

import argparse
import gzip
import io
import json
from pathlib import Path
import re
import subprocess
import tarfile
import tempfile


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def append_tree(destination, root, revision, prefix):
    process = subprocess.Popen(['git', '-C', str(root), 'archive', '--format=tar',
                                '--prefix=' + prefix, revision], stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode='r|') as source:
            for member in source:
                destination.addfile(member, source.extractfile(member) if member.isfile() else None)
    finally:
        process.stdout.close()
        result = process.wait()
    if result:
        raise RuntimeError('Could not archive the source revision.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?', args.version):
        parser.error('Invalid version.')
    root = Path(__file__).resolve().parents[2]
    if git(root, 'status', '--porcelain', '--untracked-files=normal'):
        parser.error('Commit the exact source snapshot before making a release archive.')
    revision = git(root, 'rev-parse', 'HEAD')
    pinned = git(root, 'rev-parse', 'HEAD:kBuild')
    if git(root / 'kBuild', 'rev-parse', 'HEAD') != pinned:
        parser.error('The kBuild checkout does not match the pinned revision.')
    if git(root / 'kBuild', 'status', '--porcelain', '--untracked-files=normal'):
        parser.error('The kBuild checkout has local changes.')
    if args.output.exists():
        parser.error('The output already exists.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    prefix = 'Elseport-' + args.version + '/'
    record = json.dumps({'version': args.version, 'revision': revision,
                         'submodules': {'kBuild': pinned}}, indent=2).encode() + b'\n'
    with tempfile.NamedTemporaryFile(dir=args.output.parent, prefix='source-', delete=False) as stream:
        temporary = Path(stream.name)
    try:
        with temporary.open('wb') as raw:
            with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
                with tarfile.open(fileobj=compressed, mode='w|') as archive:
                    append_tree(archive, root, revision, prefix)
                    append_tree(archive, root / 'kBuild', pinned, prefix + 'kBuild/')
                    member = tarfile.TarInfo(prefix + 'SOURCE-REVISION.json')
                    member.size = len(record)
                    member.mtime = int(git(root, 'show', '-s', '--format=%ct', revision))
                    member.mode = 0o644
                    archive.addfile(member, io.BytesIO(record))
        temporary.rename(args.output)
    finally:
        temporary.unlink(missing_ok=True)
    print(args.output.resolve())
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
