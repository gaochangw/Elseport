# Development validation

Last checked: 2026-11-01. This describes a development snapshot, not a support
promise for untested machines.

Baseline: Apple M2, macOS 27.0.1, Ubuntu 26.04.1 Arm64 and source-built Guest
Additions 7.2.97r115367. The graphical frontend uses Qt 6.12.0.

| Check | Result |
| --- | --- |
| macOS source build | Passed |
| Linux Guest Additions source build and packaging | Passed; included license and staged client checked |
| Private file cache tests | Seven groups passed, including quotas, cancellation, short reads, permissions and unsafe names |
| Private native pasteboard tests | Eight groups passed, including file URLs and ownership replacement |
| Clipboard integration | macOS and an isolated Linux X11 server: files, nested directories, Unicode names, binary bytes and empty files passed both directions |
| Repeated clipboard integration | Five immediate direction changes passed with content checks |
| Copy superseded during transfer | New clipboard content preserved; obsolete transfer canceled |
| Retina | Single GNOME Wayland monitor: 200% desktop scale, 2560×1600 framebuffer in a 1280×800 point view; all pixels matched |
| Guest-to-host file replacement | Shorter and empty files truncate correctly; no-replace preserves the destination; copied installer hash matches the guest artifact |
| Tracked-object cleanup | Explicit cleanup and destructor re-entry cases passed |
| Guest Control feature detection | Pending-message, old-host, incomplete-reply and interrupted-probe cases passed |

The file clipboard tests used a named private macOS pasteboard. They did not
replace the user's system clipboard. Real clipboard large-text and image tests
were deliberately excluded.

Finder drag and drop, external displays, fractional scaling, assistive tools,
physical keyboard shortcuts, sleep/wake and application/game performance still
require further checks. The Wayland clipboard bridge does not yet transfer
files; see [integration scope](INTEGRATION.md).

Narrow native test executables are `tstClipboardFileCache`,
`tstClipboardPasteboard`, `tstObjectsTracker` and `tstGuestCtrlPeek`. Do not run
an unrestricted clipboard test suite against a user's current clipboard.
