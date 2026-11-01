#!/usr/bin/env python3
# $Id$
## @file
# Elseport: assemble and audit a relocatable, ad-hoc-signed development disk image.
# SPDX-License-Identifier: GPL-3.0-only

import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile


MACHO_MAGIC = {b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe', b'\xfe\xed\xfa\xcf',
               b'\xcf\xfa\xed\xfe', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca',
               b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca'}


def macho_files(directory):
    for path in directory.rglob('*'):
        if path.is_file() and not path.is_symlink():
            with path.open('rb') as stream:
                if stream.read(4) in MACHO_MAGIC:
                    yield path


def ignore_development_files(directory, names):
    return [name for name in names if name in ('testcase', 'tools', '.DS_Store', 'vboxuidrive.dylib',
                                              'f-ui.dylib', 'shcl-redirect.dylib',
                                              'bldRTLdrCheckImports', 'svn2git-vbox', 'xpidl', 'VBoxAudioTest')
            or name.startswith(('tst', 'VBoxSample', 'VBoxPlayground'))
            or name.endswith(('.dSYM', '.iso', '.vbox-extpack', '.log'))]


def library_id(path):
    lines = subprocess.check_output(['/usr/bin/otool', '-D', str(path)], text=True).splitlines()
    return lines[1].strip() if len(lines) > 1 else None


def load_paths(path):
    text = subprocess.check_output(['/usr/bin/otool', '-l', str(path)], text=True)
    lines = text.splitlines()
    result = []
    for index, line in enumerate(lines):
        if line.strip() == 'cmd LC_RPATH':
            result.append(lines[index + 2].strip().split(' (offset')[0].removeprefix('path '))
    return result


def dependencies(path):
    identity = library_id(path)
    output = subprocess.check_output(['/usr/bin/otool', '-L', str(path)], text=True)
    return [line.strip().split(' (compatibility')[0] for line in output.splitlines()[1:]
            if line.strip() and line.strip().split(' (compatibility')[0] != identity]


def deploy_dependencies(app, original, qt_prefix):
    frameworks = app / 'Contents/Frameworks'
    plugins = app / 'Contents/PlugIns'
    frameworks.mkdir(exist_ok=True)
    origins = {path: original / path.relative_to(app) for path in macho_files(app)}
    copied = {}

    def resolve_reference(reference, origin):
        if reference.startswith('/System/Library/') or reference.startswith('/usr/lib/'):
            return None
        paths = []
        if reference.startswith('@loader_path/'):
            paths.append(origin.parent / reference[len('@loader_path/'):])
        elif reference.startswith('@executable_path/'):
            paths.append(original / 'Contents/MacOS' / reference[len('@executable_path/'):])
        elif reference.startswith('@rpath/'):
            tail = reference[len('@rpath/'):]
            for prefix in load_paths(origin):
                prefix = prefix.replace('@loader_path', str(origin.parent))
                prefix = prefix.replace('@executable_path', str(original / 'Contents/MacOS'))
                paths.append(Path(prefix) / tail)
            paths.extend([original / 'Contents/MacOS' / tail, qt_prefix / 'lib' / tail,
                          Path('/opt/homebrew/lib') / tail])
        elif reference.startswith('/'):
            paths.append(Path(reference))
            if '.framework/' in reference:
                name = reference.split('.framework/')[0].split('/')[-1] + '.framework/'
                paths.append(qt_prefix / 'lib' / (name + reference.split('.framework/', 1)[1]))
        for candidate in paths:
            if candidate.exists():
                return candidate.resolve()
        raise RuntimeError(f'Cannot resolve {reference} used by {origin.name}')

    def copy_library(source):
        source = source.resolve()
        if source.is_relative_to(original.resolve()):
            return app / source.relative_to(original.resolve())
        if source in copied:
            return copied[source]
        framework = next((parent for parent in source.parents if parent.suffix == '.framework'), None)
        if framework:
            target_root = frameworks / framework.name
            if not target_root.exists():
                shutil.copytree(framework, target_root, symlinks=True,
                                ignore=lambda directory, names: [name for name in names
                                      if name in ('Headers', 'Modules') or name.endswith(('.dSYM', '.prl'))])
                for binary in macho_files(target_root):
                    origins[binary] = framework / binary.relative_to(target_root)
            target = target_root / source.relative_to(framework)
        else:
            target = frameworks / source.name
            if target.exists():
                if hashlib.sha256(target.read_bytes()).digest() != hashlib.sha256(source.read_bytes()).digest():
                    raise RuntimeError(f'Different libraries have the same filename: {source.name}')
            else:
                shutil.copy2(source, target)
            origins[target] = source
        copied[source] = target
        return target

    for category in ('platforms', 'imageformats', 'iconengines', 'styles', 'printsupport', 'tls', 'networkinformation'):
        source_dir = qt_prefix / 'plugins' / category
        if source_dir.is_dir():
            for source in source_dir.glob('*.dylib'):
                target = plugins / category / source.name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
                origins[target] = source.resolve()

    processed = set()
    while True:
        queue = [path for path in origins if path not in processed]
        if not queue:
            break
        for target in queue:
            processed.add(target)
            origin = origins[target]
            target.chmod(target.stat().st_mode | 0o200)
            changes = []
            for reference in dependencies(origin):
                source = resolve_reference(reference, origin)
                if source is None:
                    continue
                destination = copy_library(source)
                replacement = '@loader_path/' + os.path.relpath(destination, target.parent)
                changes += ['-change', reference, replacement]
            identity = library_id(origin)
            if identity:
                relative = target.relative_to(frameworks) if target.is_relative_to(frameworks) else Path(target.name)
                changes += ['-id', '@rpath/' + str(relative)]
            for value in load_paths(target):
                if value.startswith('/') and not value.startswith(('/System/', '/usr/lib/')):
                    changes += ['-delete_rpath', value]
            if changes:
                subprocess.run(['/usr/bin/install_name_tool', *changes, str(target)], check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

    resources = app / 'Contents/Resources'
    (resources / 'qt.conf').write_text('[Paths]\nPlugins=PlugIns\n')
    for nested in resources.glob('*.app'):
        directory = nested / 'Contents/Resources'
        directory.mkdir(exist_ok=True)
        (directory / 'qt.conf').write_text('[Paths]\nPlugins=../../../PlugIns\n')
    return len(processed)


def audit_bundle(app):
    failures = []
    inventory = []
    for path in macho_files(app):
        linked = dependencies(path)
        for dependency in linked:
            if dependency.startswith('/') and not dependency.startswith(('/System/Library/', '/usr/lib/')):
                failures.append(f'{path.relative_to(app)}: external load command {dependency}')
            if dependency.startswith('@loader_path/'):
                resolved = (path.parent / dependency[len('@loader_path/'):]).resolve()
                if not resolved.is_relative_to(app.resolve()) or not resolved.exists():
                    failures.append(f'{path.relative_to(app)}: missing bundled dependency {dependency}')
            elif dependency.startswith('@'):
                failures.append(f'{path.relative_to(app)}: unresolved load command {dependency}')
        for value in load_paths(path):
            if value.startswith('/') and not value.startswith(('/System/', '/usr/lib/')):
                failures.append(f'{path.relative_to(app)}: external search path {value}')
        inventory.append({'path': str(path.relative_to(app)), 'dependencies': linked,
                          'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    for path in app.rglob('*'):
        if path.is_symlink():
            if not path.resolve().is_relative_to(app.resolve()):
                failures.append(f'{path.relative_to(app)}: symlink points outside the bundle')
            elif not path.exists():
                failures.append(f'{path.relative_to(app)}: broken symlink')
    if failures:
        raise RuntimeError('Bundle is not relocatable:\n' + '\n'.join(failures))
    return inventory


def minimum_macos(app):
    versions = [(11, 0)]
    for path in macho_files(app):
        lines = subprocess.check_output(['/usr/bin/otool', '-l', str(path)], text=True).splitlines()
        build = False
        for line in lines:
            value = line.strip()
            if value.startswith('cmd '):
                build = value == 'cmd LC_BUILD_VERSION'
            if build and value.startswith('minos '):
                versions.append(tuple(int(part) for part in value.split()[1].split('.')))
    return '.'.join(str(part) for part in max(versions))


def relocate_runtime_data(app):
    """Keep data out of the directory that codesign interprets as nested code."""
    binary_dir = app / 'Contents/MacOS'
    data_dir = app / 'Contents/Resources/RuntimeData'
    for path in list(binary_dir.rglob('*')):
        if not path.is_file() or path.is_symlink() or path.name == 'Elseport':
            continue
        with path.open('rb') as stream:
            is_code = stream.read(4) in MACHO_MAGIC
        if is_code:
            continue
        target = data_dir / path.relative_to(binary_dir)
        target.parent.mkdir(parents=True, exist_ok=True)
        path.rename(target)
        path.symlink_to(os.path.relpath(target, path.parent))


def materialize_helper_executables(app):
    """Give a helper its own executable directory rather than aliasing the manager's."""
    main = app / 'Contents/MacOS'
    for bundle in (app / 'Contents/Resources').glob('*.app'):
        executable_dir = bundle / 'Contents/MacOS'
        if not executable_dir.is_symlink():
            continue
        with (bundle / 'Contents/Info.plist').open('rb') as stream:
            executable = plistlib.load(stream)['CFBundleExecutable']
        source = (executable_dir / executable).resolve()
        content = source.read_bytes()
        mode = source.stat().st_mode
        executable_dir.unlink()
        executable_dir.mkdir()
        target = executable_dir / executable
        target.write_bytes(content)
        target.chmod(mode)
        alias = main / executable
        if alias.exists() and alias.read_bytes() == content:
            alias.unlink()
            alias.symlink_to(os.path.relpath(target, alias.parent))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[2]
    parser.add_argument('--app', type=Path, default=root / 'out/darwin.arm64/release/dist/VirtualBox.app')
    parser.add_argument('--output', type=Path, default=root / 'prebuild/release')
    parser.add_argument('--qt-prefix', type=Path, default=root / 'prebuild/qt6')
    parser.add_argument('--entitlements', type=Path, default=root / 'out/darwin.arm64/release/EntitlementsVM.plist')
    parser.add_argument('--audit-only', type=Path)
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('This packaging tool requires macOS.')
    if args.audit_only:
        inventory = audit_bundle(args.audit_only.resolve())
        print(json.dumps({'macho_files': len(inventory), 'relocatable': True}))
        return 0
    for path in (args.app, args.qt_prefix, args.entitlements):
        if not path.exists():
            parser.error(f'Missing prerequisite: {path}')
    args.output.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='development-', dir=args.output.resolve()))
    stage = work / 'image'
    stage.mkdir()
    app = stage / 'Elseport Development.app'
    shutil.copytree(args.app, app, symlinks=True, ignore=ignore_development_files)
    # Development snapshots must not claim an existing installation's file types.
    for info in [app / 'Contents/Info.plist', *(app / 'Contents/Resources').glob('*.app/Contents/Info.plist')]:
        with info.open('rb') as stream:
            metadata = plistlib.load(stream)
        metadata.pop('CFBundleDocumentTypes', None)
        metadata.pop('CFBundleURLTypes', None)
        with info.open('wb') as stream:
            plistlib.dump(metadata, stream)
    materialize_helper_executables(app)
    contents = app / 'Contents'

    # Build the launcher after deploying the original application's dependencies.
    launcher = contents / 'MacOS/Elseport'
    plist_path = contents / 'Info.plist'
    with plist_path.open('rb') as stream:
        plist = plistlib.load(stream)
    plist.update(CFBundleName='Elseport Development',
                 CFBundleDisplayName='Elseport Development', CFBundleIdentifier='org.elseport.development',
                 CFBundleIconFile='Elseport.icns')
    with plist_path.open('wb') as stream:
        plistlib.dump(plist, stream)
    shutil.copy2(root / 'brand/Elseport.icns', contents / 'Resources/Elseport.icns')

    deploy_dependencies(app, args.app.resolve(), args.qt_prefix.resolve())
    compiler = subprocess.check_output(['/usr/bin/xcrun', '--find', 'clang'], text=True).strip()
    sdk = subprocess.check_output(['/usr/bin/xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
    subprocess.run([compiler, '-Os', '-Wall', '-Wextra', '-Werror', '-arch', 'arm64',
                    '-mmacosx-version-min=11.0', '-isysroot', sdk,
                    str(root / 'tools/elseport/development-launcher.c'), '-o', str(launcher)], check=True)
    # Keep dependency discovery rooted at the original executable until deployment
    # is finished, then select the native launcher which isolates development settings.
    plist['CFBundleExecutable'] = 'Elseport'
    plist['LSMinimumSystemVersion'] = minimum_macos(app)
    with plist_path.open('wb') as stream:
        plistlib.dump(plist, stream)
    relocate_runtime_data(app)
    inventory = audit_bundle(app)
    subprocess.run(['/bin/bash', str(root / 'tools/elseport/sign-development.sh'), str(app),
                    str(args.entitlements.resolve())], check=True)
    for entry in inventory:
        entry['sha256'] = hashlib.sha256((app / entry['path']).read_bytes()).hexdigest()
    (work / 'bundle-manifest.json').write_text(json.dumps(inventory, indent=2) + '\n')
    try:
        revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
        difference = subprocess.check_output(['git', 'diff', '--binary', 'HEAD'], cwd=root)
    except (OSError, subprocess.CalledProcessError):
        revision, difference = 'unknown', b''
    (work / 'build-record.json').write_text(json.dumps({
        'base_revision': revision,
        'tracked_diff_sha256_at_packaging': hashlib.sha256(difference).hexdigest(),
        'minimum_macos': plist['LSMinimumSystemVersion'],
        'distribution': 'internal-development',
        'signature': 'ad-hoc',
        'notarized': False,
        'source_release_prepared': False,
    }, indent=2) + '\n')
    readme = '''Elseport development build

This is an internal, ad-hoc-signed test build, not a notarized public release.
The existing frontend is included; the new Elseport interface is still a design prototype.
This image is not prepared for public distribution; release-scope, source, licensing and security work remain.
The launcher uses a separate Elseport-Development configuration directory and IPC identifier.
This development bundle does not register virtual-machine file associations; open computers from the manager.
For tests, override VBOX_USER_HOME with the repository's prebuild/vbox-home directory.
'''
    (stage / 'READ-ME.txt').write_text(readme)
    for name in ('COPYING', 'LICENSE.txt', 'NOTICE'):
        if (root / name).is_file():
            shutil.copy2(root / name, stage / name)
    (stage / 'Applications').symlink_to('/Applications')
    image = work / 'Elseport-Development.dmg'
    subprocess.run(['/usr/bin/hdiutil', 'create', '-fs', 'HFS+', '-format', 'UDZO', '-volname',
                    'Elseport Development', '-srcfolder', str(stage), str(image)], check=True)
    print(json.dumps({'image': str(image), 'bundle': str(app), 'macho_files': len(inventory),
                      'signature': 'ad-hoc', 'notarized': False}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
