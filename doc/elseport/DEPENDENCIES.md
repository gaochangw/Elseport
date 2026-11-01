# Dependency and license notes

This is a development inventory, not a complete release SBOM. The final
distributable must be checked recursively, including plugins and static code.

| Component | Current development evidence | License work |
| --- | --- | --- |
| VirtualBox base | 7.2.97r175247 plus local modifications | GPLv3; retain upstream notices and provide exact corresponding source |
| Qt | Local GUI links Qt 6.12.0 Core, Gui, Widgets, PrintSupport, Help and StateMachine | Review the selected GPL/LGPL route, each module, plugins, patches and transitive dependencies |
| Manrope | Bundled for the design and wordmark | SIL Open Font License 1.1, original notice under `brand/type/` |

The installed Qt also links third-party libraries such as ICU, GLib, OpenSSL,
PCRE2, double-conversion, libb2 and zstd. A package inventory must record the
actual versions and included notices; the top-level Qt version is insufficient.
The development packaging helper audits load paths, not license compliance.

Sources of authority are `COPYING`, each component's license files, the pinned
source archives and the actual bundle inventory. Existing manual license tables
may describe different component versions. Do not silently reuse them as a
claim about this build.

System frameworks are used from macOS, not copied into the application. No
proprietary Extension Pack or guest operating-system installation media belongs
in the package. Build tools and source-download locations must remain documented
without redistributing toolchains whose terms do not allow it.
