# Bundled dependencies

The preview bundles the shared libraries listed below. Exact upstream archives,
Homebrew build recipes and formula patches accompany each binary release in
the dependency-source archive. [dependencies.json](dependencies.json) records
source URLs and SHA-256 values; component notices are also inside the app at
`Contents/Resources/Licenses/third-party`.

| Component | Packaged version |
| --- | --- |
| brotli | 1.2.1 |
| dbus | 1.16.2_1 |
| double-conversion | 3.4.0 |
| freetype | 2.15.0 |
| gettext | 1.0 |
| glib | 2.90.1 |
| graphite2 | 1.3.15 |
| harfbuzz | 15.0.0_1 |
| icu4c@79 | 79.1 |
| jpeg-turbo | 3.2.0 |
| libb2 | 0.98.1 |
| libpng | 1.6.59 |
| md4c | 0.6.0 |
| openssl@4 | 4.1.0 |
| pcre2 | 10.49 |
| qtbase | 6.12.0_2 |
| qtscxml | 6.12.0_1 |
| qtsvg | 6.12.0_1 |
| qttools | 6.12.0_1 |
| zstd | 1.5.7_1 |

The project source archive includes the pinned kBuild source, built-in libraries
and their original license notices. Manrope is used in the design and wordmark
under the SIL Open Font License 1.1, with its notice in `brand/type/`.

The packaging helper recursively inventories Mach-O load commands, includes
the selected Qt plugins and rejects external non-system libraries. Its bundle
manifest lists the exact files and hashes. macOS system frameworks and Apple
development tools are used from the host and are not redistributed.

No proprietary Extension Pack or guest operating-system ISO is bundled. The
Linux integration ISO contains this project’s source-built Guest Additions;
Ubuntu installation media is downloaded separately by the user.

Covered project code and the selected Qt distribution follow GNU GPL version 3;
third-party components retain their own licenses. The source archives and
original license files, rather than the table alone, define those terms.
