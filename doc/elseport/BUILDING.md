# Building an Elseport development snapshot

These instructions describe the tested local development workflow on an Apple
Silicon Mac. They are not a notarized installer or a guarantee for other hosts.
The interface design in `doc/design/` is separate from the existing frontend.

## Prerequisites

- Apple Command Line Tools, with an installed macOS SDK.
- The repository's pinned kBuild checkout.
- NASM and YASM as required by the existing firmware targets.
- Qt 6 modules used by the GUI: Core, Gui, Widgets, PrintSupport, Help and
  StateMachine. The current local build uses Qt 6.12.0 from Homebrew.

Install prerequisites yourself using the applicable upstream build guidance.
The helper scripts do not install packages, accept toolchain licenses, change
`xcode-select`, or invoke `sudo`.

## Configure and build

External development uses `configure.py`. Read the host-specific
[upstream instructions](https://www.virtualbox.org/wiki/Build_instructions)
before configuration. Keep machine paths and optional test settings in the
ignored `LocalConfig.kmk`; do not copy another machine's absolute paths.

The generated environment is used for the build:

```sh
DEVELOPER_DIR=/Library/Developer/CommandLineTools /bin/bash -c '. ./env.sh; kmk'
```

Use `KBUILD_TYPE=release` or `KBUILD_TYPE=debug` explicitly when necessary.
Output is under `out/<host>.<arch>/<build-type>/`. Do not rebuild libraries
while a VM is using them.

## Development package

Make a separate copy before relocation or signing. The helper excludes tests,
debug symbols and cached installation media, copies the required Qt plugins and dependency closure, rejects
remaining external library paths, and signs the copied bundle ad hoc:

```sh
DEVELOPER_DIR=/Library/Developer/CommandLineTools \
python3 tools/elseport/package-development.py \
    --qt-prefix /path/to/merged/qt-prefix
```

Use `--app`, `--output`, `--qt-prefix` and `--entitlements` for a non-default build.
The Qt prefix must expose `lib/` and `plugins/`; it can merge installed Qt
modules using local symlinks. Only copies are modified during packaging. Each run
gets a new staging directory instead of overwriting an existing application.
The result is named **Elseport Development** and retains the existing frontend.
It is not signed with a Developer ID and is not notarized.

The launcher sets the component search path to this bundle and uses a separate
development settings directory and IPC identifier. The bundle's minimum macOS
version is calculated from its dependencies rather than copied from the host
target's deployment flag.
Development bundles do not register virtual-machine file associations; open
computers from the manager.
For all repository tests, explicitly override its environment:

```sh
export VBOX_USER_HOME="$PWD/prebuild/vbox-home"
export VBOX_IPC_SOCKETID=elseport-tests
```

The launcher must not import or migrate an existing application's settings.
Keep test computers under `prebuild/vms`. Never use development tests to write
to the user's real clipboard without explicit, current authorization.

## Signing boundary

`tools/elseport/sign-development.sh` operates only on the supplied bundle copy.
It signs VM executables with the provided hypervisor entitlements and checks
the resulting bundle. This is a local test mechanism, not proof of public
distribution readiness. Developer ID signing and notarization require separate
identity, credential, hardened-runtime and validation work.

For source publication, follow the source checklist in [RELEASE.md](RELEASE.md).
Before distributing a binary, provide its exact corresponding source and
dependency notices; a source-tree link alone does not identify that snapshot.

## Linux Guest Additions development

Use a separate, complete checkout inside an isolated Ubuntu Arm64 test machine,
including kBuild. Do not reuse the Mac's generated configuration files.
The tested Ubuntu 26.04.1 setup needed a C/C++ compiler, matching kernel headers,
Perl, Make, pkg-config, xsltproc, makeself and NASM, plus development headers for
libcap, PAM, ALSA, device-mapper, PulseAudio, OpenGL, X11, XRandR, Xinerama, Xmu,
Xcursor, Xfixes, Xrender, xkbcommon, Wayland and DRM. The configure script reports
missing dependencies; these instructions do not install them automatically.

From that guest's source directory:

```sh
python3 configure.py --only-additions \
    --with-kbuild-path "$PWD/kBuild/kBuild" \
    --disable-qt --disable-sdl --disable-java --disable-python \
    --disable-docs --disable-opengl --disable-hardening
. ./env.sh
kmk -j2
kmk -j2 "$PWD/out/linux.arm64/release/bin/additions/VBoxLinuxAdditions.run"
```

For an SVN layout, point the kBuild option at the directory containing its
`header.kmk` and `bin/`. Run the normal build before the installer target so that
its data and license files are staged. Check that `LICENSE` is present in
`VBoxGuestAdditions-arm64.tar.bz2`, then test installation and the kernel modules
in the dedicated guest. This is a development recipe; it does not make the
result a supported or publicly distributable release.

The Linux installer can return status 2 when an older kernel module was still
in use. Reboot the test guest and check the loaded module and user-tool versions;
do not treat an arbitrary non-zero installation result as success.
