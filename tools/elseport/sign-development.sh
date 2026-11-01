#!/bin/bash
# $Id$
## @file
# Elseport: ad-hoc sign an isolated development bundle without changing system settings.
# SPDX-License-Identifier: GPL-3.0-only

set -euo pipefail
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <copied-app-bundle> <EntitlementsVM.plist>" >&2
    exit 2
fi
elseport_app=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
elseport_entitlements=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
test -d "$elseport_app/Contents"
test -f "$elseport_entitlements"

python3 - "$elseport_app" "$elseport_entitlements" <<'PY'
from pathlib import Path
import plistlib
import subprocess
import sys

app = Path(sys.argv[1])
entitlements = Path(sys.argv[2])
magic = {b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf',
         b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca',
         b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca'}
for path in sorted(app.rglob('*'), key=lambda item: len(item.parts), reverse=True):
    if not path.is_file() or path.is_symlink():
        continue
    with path.open('rb') as stream:
        is_macho = stream.read(4) in magic
    if not is_macho:
        continue
    command = ['/usr/bin/codesign', '--force', '--sign', '-']
    if path.name in ('VirtualBoxVM', 'VBoxHeadless'):
        command += ['--entitlements', str(entitlements)]
    subprocess.run(command + [str(path)], check=True, stdout=subprocess.DEVNULL)
bundles = [path for path in app.rglob('*') if path.is_dir() and path.suffix in ('.app', '.framework', '.bundle', '.plugin')]
for bundle in sorted(bundles, key=lambda item: len(item.parts), reverse=True) + [app]:
    command = ['/usr/bin/codesign', '--force', '--sign', '-']
    info = bundle / 'Contents/Info.plist'
    if info.is_file():
        executable = plistlib.loads(info.read_bytes()).get('CFBundleExecutable')
        if executable in ('VirtualBoxVM', 'VBoxHeadless'):
            # Signing the enclosing bundle replaces the executable signature.
            command += ['--entitlements', str(entitlements)]
    subprocess.run(command + [str(bundle)], check=True, stdout=subprocess.DEVNULL)
subprocess.run(['/usr/bin/codesign', '--verify', '--deep', '--strict', str(app)], check=True)
expected = plistlib.loads(entitlements.read_bytes())
for binary in app.rglob('*'):
    if binary.name in ('VirtualBoxVM', 'VBoxHeadless') and binary.is_file() and not binary.is_symlink():
        data = subprocess.check_output(['/usr/bin/codesign', '--display', '--entitlements', '-', '--xml', str(binary)],
                                       stderr=subprocess.DEVNULL)
        actual = plistlib.loads(data)
        if any(actual.get(key) != value for key, value in expected.items()):
            raise RuntimeError(f'Missing VM entitlements: {binary}')
print('Verified ad-hoc development signature; this is not a notarized release.')
PY
