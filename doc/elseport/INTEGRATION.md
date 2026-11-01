# Desktop integration

These are development features, validated on an Apple M2 with macOS 27.0.1
and an Ubuntu 26.04.1 Arm64 guest. Install Guest Additions built from the same
source snapshot to use the Linux changes. The upstream prebuilt Additions
download does not contain them.

## File clipboard

Enable the desired Shared Clipboard direction and **Clipboard File Transfers**
for the individual VM. Keep sharing disabled for untrusted guests.

The macOS receiver and Linux X11 receiver copy the complete offer to a private
directory before announcing local file URLs. Multiple files, directories,
Unicode names and empty files are supported. Symlinks, special files, unsafe
names, incomplete reads and over-limit offers are rejected.

Limits per offer are 1 GiB of file data, 16,384 entries and 64 directory levels.
The cache has an 8 GiB aggregate limit and reserves 256 MiB of free space. Files
use owner-only permissions. Partial and superseded macOS copies are discarded.
Completed copies persist after the VM exits so that a pending paste can still
use them.

Cache locations:

- macOS: `VBoxClipboardCache` below `VBOX_USER_HOME`; without that environment
  override, below the current user's temporary directory.
- Linux guest: `~/.vbox-clipboard`.

There is no automatic expiry for completed copies. When the cache is full,
finish any pending paste, close applications using the copied files, stop the
VMs sharing that cache, and remove the cache directory. Do not clear it while
a paste or file transfer is active.

The Linux implementation currently targets X11 clipboard clients. The
Wayland clipboard bridge does not yet support these file transfers. Directory
transfers use local file URLs rather than HTTP directory links.

Automated integration checks use a named private macOS pasteboard and an
isolated X server in the guest. The user's system clipboard was excluded.
Finder drag and drop still needs a manual interoperability check; the Qt
conversion and macOS drag preparation paths have separate local tests.

## Retina display

With one guest monitor and a running GNOME Wayland session, current Guest
Additions advertise automatic desktop scaling. If no explicit host zoom is
selected, the frontend requests the host display's integer scale (100%, 200%
or 300%). The guest chooses a supported physical mode with a suitable logical
size and applies the scale for the current login session.

The frontend uses native pixels only after an acknowledgement for its current
request. A stale acknowledgement cannot enable it. Other desktops, older
Guest Additions, multiple guest monitors and explicit host zoom retain the
existing display behaviour. External monitors and fractional scales need
additional validation.

At 200%, the tested guest rendered at 2560×1600 in a 1280×800 point view. The
captured view and guest framebuffer matched at every pixel.
