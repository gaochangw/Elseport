# Elseport

**Linux workspaces for your Mac.**

Elseport is an independent desktop virtualisation project for Apple Silicon,
built from the open-source code of VirtualBox. Create an Ubuntu computer, open
its desktop, and pick up where you left off through a native macOS interface.

[简体中文](README.zh-CN.md) · [Download preview](https://github.com/gaochangw/Elseport/releases) · [Build](doc/elseport/BUILDING.md) · [Security](SECURITY.md)

![Elseport native interface in English, light appearance](doc/elseport/previews/native-en.png)

*The native application, showing a real Linux computer. Light appearance only.*

## Try the preview

The first preview targets **Apple Silicon Macs with macOS 27.0 or later** and
Linux Arm64 guests. It has been tested on Apple M2 with macOS 27.0.1 and Ubuntu
26.04.1 Arm64. Other configurations need their own validation.

1. Download the macOS Arm64 disk image from [Releases](https://github.com/gaochangw/Elseport/releases).
2. Open the image and copy **Elseport.app** into Applications.
3. Open Elseport, choose **New computer**, and create an Ubuntu account.
   Leave the image field empty to download and verify Ubuntu automatically,
   or select a Linux Arm64 ISO you already have.

Keep at least 20 GB of free space for a new Ubuntu installation. The default
virtual disk grows as it is used; its 64 GB limit is not allocated up front.
Automatic setup needs an internet connection for Ubuntu packages and updates.

This is an **alpha preview**, signed ad hoc and **not notarized**. macOS may
block a downloaded copy; see the [installation and release notes](doc/elseport/RELEASE.md).
It is not an App Store build or a production-supported distribution.

## What works

- A native English/Chinese interface with your computers, live desktop previews
  and real power states.
- Guided Ubuntu installation with a checked download, a chosen account and
  bundled Linux integration tools. Other Linux Arm64 images can be installed
  manually.
- Start, pause, resume, save and close, normal shutdown and restore points.
- Memory and processor settings, display scaling and an explicitly selected
  shared folder. Clipboard and folder sharing start disabled; shared folders
  are read-only unless you allow changes.

Read the [native interface guide](doc/elseport/NATIVE-UI.md) and
[validation record](doc/elseport/VALIDATION.md) for the tested scope. Settings
are stored separately from an existing VirtualBox installation; the preview
does not import existing computers automatically.

## Current limits

- This build uses software graphics. Application and game 3D compatibility is
  not supported by this preview.
- File clipboard requires an X11 guest desktop; native Wayland file transfer
  is not supported. See [desktop integration](doc/elseport/INTEGRATION.md).
- External displays, accessibility tools, host sleep/wake and physical input
  devices need more hands-on testing.
- Restart Elseport after changing the interface language.
- The proprietary Extension Pack is not included or required.

## Source and development

Follow [BUILDING.md](doc/elseport/BUILDING.md) for the macOS host, Linux Guest
Additions and packaging workflow. Each binary release includes the matching
project source, dependency source archives and checksums. The
[dependency notes](doc/elseport/DEPENDENCIES.md) describe their scope.

The earlier [interactive design prototype](doc/design/DESIGN.md) remains a
local design reference. The application uses Qt Widgets and the existing
virtual-machine API; it does not embed the HTML prototype.

## Origin and license

This work starts from VirtualBox `7.2.97r175247`, Git mirror revision
`b6bea72111fcff48f31c57c9f66365b1d3301bb9`. The canonical upstream repository
is SVN; its Git mirror may lag it.

Covered code is licensed under GNU GPL version 3. Third-party components retain
their own licenses. See [COPYING](COPYING), [LICENSE.txt](LICENSE.txt),
[NOTICE](NOTICE) and [dependency notes](doc/elseport/DEPENDENCIES.md). Existing
copyright and license notices are retained.

Elseport is not affiliated with, endorsed by or sponsored by Oracle. Oracle
and VirtualBox are trademarks of Oracle and/or its affiliates. Mac and macOS
are trademarks of Apple Inc. Ubuntu is a trademark of Canonical Ltd. References
identify origin or compatibility context, not endorsement.

Read [CONTRIBUTING.md](CONTRIBUTING.md) before preparing a change. Do not disclose
possible vulnerabilities in public issues, patches or comments; follow
[SECURITY.md](SECURITY.md).
