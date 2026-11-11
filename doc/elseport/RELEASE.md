# Installing the macOS preview

The first binary preview targets Apple Silicon Macs, macOS 27.0 or later,
and Linux Arm64 guests. The validated host is an Apple M2 running macOS
27.0.1; the validated guest is Ubuntu 26.04.1 Arm64. This is an alpha release
with software graphics, not a production-supported distribution.

## Install and open

1. Download the macOS Arm64 DMG and `SHA256SUMS` from the same GitHub release.
   Compare the output of `shasum -a 256 <downloaded-file.dmg>` with its entry.
2. Open the disk image and drag **Elseport.app** to Applications.
3. Open Elseport and create a new computer. The application uses its own
   configuration directory and does not import an existing installation.
4. Keep at least 20 GB free for a new Ubuntu installation. Clipboard and shared
   folders remain disabled until you explicitly enable them.

The preview is **ad-hoc signed, not Developer ID signed, and not notarized**.
A downloaded copy may be blocked by macOS. If you choose to trust this release
after checking its source and checksum, Apple's per-app approval is in
**System Settings → Privacy & Security → Security → Open / Open Anyway** after
an attempted launch. Consult [Apple's current instructions](https://support.apple.com/en-us/102445).
Managed Macs may not permit this approval. The installer does not disable
Gatekeeper or change system-wide security settings.

Local installation testing covers mounting the DMG, copying the app, starting
it from the installed location and using dedicated Linux computers. It does
not establish that a downloaded, quarantined copy will open without approval
on another Mac. Physical input devices, accessibility, external monitors and
host sleep/wake need additional manual coverage.

## Data locations

The release launcher uses `~/Library/Application Support/Elseport` for its
configuration, with a separate service identifier. Computers created from a
fresh profile are stored under its `Computers` directory. The Ubuntu cache is
inside the profile's `Cache` directory. Removing the app does not delete these
computers or their files.

Automatic installation temporarily generates private setup media in the new
computer's directory. After the installed guest confirms that its integration
service has started, the application ejects its own setup media and removes
the generated files. A canceled download can be resumed; external ISO files
selected by the user are retained.

## Release contents

Each binary release must include:

- The versioned macOS Arm64 DMG and Linux-only integration ISO.
- Exact project source, including the pinned kBuild source and build scripts.
- Dependency source archives, original build recipes, patches and notices.
- A bundle inventory, source/build record and `SHA256SUMS`.

Source, system and licensing notes are in [BUILDING.md](BUILDING.md),
[DEPENDENCIES.md](DEPENDENCIES.md), [COPYING](../../COPYING) and
[NOTICE](../../NOTICE). No proprietary Extension Pack is distributed.

## Maintainer verification

Before publishing a new binary:

1. Build from the reviewed public source with a clean working tree. Keep
   private experiments, credentials, virtual disks and local configuration out.
2. Build Linux Guest Additions from that source, inspect its license contents,
   and exercise installation in a dedicated guest.
3. Test native creation, cancellation, lifecycle, settings and restore points.
   Check sharing defaults and removal of generated installation credentials.
4. Package a copy, audit all non-system dependencies and signatures, mount the
   DMG, install it locally and test the installed copy with a dedicated profile.
5. Provide matching source and notices, generate checksums, upload the release
   assets, and verify the published assets against the local hashes.

Do not rebuild host libraries while a test VM is using them. Tests must use
isolated profiles and computers. Only replace the real system clipboard when
its owner has explicitly authorized that test. Report potential vulnerabilities
through [SECURITY.md](../../SECURITY.md), not public release notes or issues.

## 中文安装提示

下载同一 Release 中的 Arm64 DMG 和校验文件，核对 SHA-256 后打开映像，
将 Elseport.app 拖入 Applications。本预览版使用本地签名、未公证；
macOS 可能阻止打开。确认信任该版本后，可按上面的 Apple 官方说明，
在“系统设置 → 隐私与安全性”中仅为这个应用批准打开。
安装程序不会关闭系统级安全检查。

应用使用独立配置，卸载应用不会自动删除虚拟电脑。建议为 Ubuntu 安装
预留至少 20 GB；共享功能默认关闭。本版只验证了 Apple M2、macOS 27.0.1
和 Ubuntu 26.04.1 Arm64，其他环境、外接显示器、辅助功能与睡眠唤醒仍需验证。
