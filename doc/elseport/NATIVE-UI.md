# Elseport native preview

The first native preview targets Apple Silicon Macs and Linux Arm64 guests.
The interface uses Qt Widgets and the existing virtual-machine API. The HTML
prototype remains a design reference; it is not embedded in the application.

## What is connected

- **Computers:** registered Linux computers, their real power state, resource
  settings and a desktop preview when the computer is running or saved.
- **Create:** download and SHA-256 verification of Ubuntu 26.04 LTS Arm64,
  resumable partial downloads, or a user-selected Linux Arm64 ISO.
- **Automatic Ubuntu setup:** account, memory, processors and dynamic disk;
  the bundled Linux integration tools are installed with the guest.
- **Lifecycle:** start/open, pause/resume, save and close, normal shutdown and
  a separately confirmed force power-off.
- **Settings:** memory, processor count, clipboard direction, file transfers,
  one explicitly selected shared folder, and display scaling. Configuration
  changes require the computer to be shut down.
- **Restore points:** create a snapshot and restore a selected snapshot after
  confirmation. Restore requires a stopped computer.
- **About:** project origin, license, source and build instructions.

The English and Chinese interface can be selected in the sidebar. Reopen the
application after changing language to update all controls.

## Data and sharing

The release launcher uses `~/Library/Application Support/Elseport` and a
separate service identifier. It does not automatically import another
installation's settings. New computers have clipboard and folder sharing off.
A shared folder is read-only unless the user explicitly enables changes.

After automatic installation, a guest first-boot service confirms completion.
The application then removes that computer's generated setup media and
credential-bearing files. A canceled or failed setup retains the downloaded
image so it can be reused; a selected external ISO is never deleted.

## Current limits

This is a preview release. The build is ad-hoc signed and not notarized.
Only Apple M2 with macOS 27.0.1 and Ubuntu 26.04.1 Arm64 have been validated;
the bundled Qt libraries require macOS 27.0 or later.

The preview uses software graphics. It does not promise application or game
3D performance. File clipboard support is limited to the X11 backend; native
Wayland file transfer remains unsupported. External displays, assistive tools
and sleep/wake need additional coverage. The existing runtime window handles
keyboard, pointer and guest display integration.
