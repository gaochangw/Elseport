# Elseport

**在 Mac 上，拥有自己的 Linux 工作空间。**

Elseport 是面向 Apple Silicon 的独立桌面虚拟化项目，基于 VirtualBox
开源代码开发。通过原生 macOS 界面创建 Ubuntu 电脑、打开桌面，再从上次
停下的地方继续工作。

[English](README.md) · [下载预览版](https://github.com/gaochangw/Elseport/releases) · [构建](doc/elseport/BUILDING.md) · [安全](SECURITY.md)

![Elseport 中文原生界面，浅色外观](doc/elseport/previews/native-zh-CN.png)

*实际应用中的 Linux 电脑，仅展示浅色外观。*

## 试用预览版

首个预览版面向 **macOS 27.0 或更新系统的 Apple Silicon Mac**，客体为
Linux Arm64。已测试 Apple M2、macOS 27.0.1 与 Ubuntu 26.04.1 Arm64；
其他配置仍需单独验证。

1. 从 [Releases](https://github.com/gaochangw/Elseport/releases) 下载 macOS Arm64 磁盘映像。
2. 打开映像，将 **Elseport.app** 拖入 Applications。
3. 打开 Elseport，选择“创建虚拟电脑”，设置 Ubuntu 账户。
   镜像栏留空即可自动下载并校验 Ubuntu，也可以选择已有的 Linux Arm64 ISO。

建议为全新 Ubuntu 安装预留至少 20 GB 空间。默认虚拟磁盘按实际使用增长，
不会一开始就占满 64 GB 上限。
自动安装需要联网获取 Ubuntu 软件包与更新。

这是 **alpha 预览版**，使用本地签名，**未进行公证**。macOS 可能阻止打开
下载的副本，请阅读[安装与发布说明](doc/elseport/RELEASE.md)。
它不是 App Store 版本，也不是提供生产环境支持的发行版。

## 已接入的功能

- 原生中英文界面，显示真实电脑列表、桌面预览与运行状态。
- 引导式 Ubuntu 安装：下载校验、自选账户、内置 Linux 增强工具。
  其他 Linux Arm64 镜像可手动安装。
- 启动、暂停、继续、保存并关闭、正常关机与恢复点。
- 内存、处理器、显示缩放以及指定目录共享。剪贴板和目录共享默认关闭；
  共享目录默认只读，需明确开启后才能修改文件。

测试范围见[原生界面说明](doc/elseport/NATIVE-UI.md)与
[验证记录](doc/elseport/VALIDATION.md)。应用使用独立配置目录，
不会自动导入已有 VirtualBox 安装中的电脑。

## 当前限制

- 本版使用软件图形，应用和游戏的 3D 兼容性不在支持范围内。
- 文件剪贴板需要 X11 客体桌面，不支持原生 Wayland 文件传输。
  详见[桌面集成说明](doc/elseport/INTEGRATION.md)。
- 外接显示器、辅助功能、宿主睡眠唤醒与实体输入设备仍需更多人工测试。
- 修改界面语言后需要重新打开 Elseport。
- 不包含、也不要求安装专有 Extension Pack。

## 源码与开发

[构建指南](doc/elseport/BUILDING.md)包含 macOS 宿主、Linux 增强工具和打包流程。
每个二进制版本同时提供对应项目源码、依赖源码与校验值，范围见
[依赖说明](doc/elseport/DEPENDENCIES.md)。

早期[交互设计原型](doc/design/DESIGN.md)保留为本地设计参考。
实际应用使用 Qt Widgets 和现有虚拟机接口，没有嵌入 HTML 原型。

## 来源与许可证

本项目起始于 VirtualBox `7.2.97r175247`，Git 镜像版本
`b6bea72111fcff48f31c57c9f66365b1d3301bb9`。上游以 SVN 为准，Git 镜像可能滞后。

受覆盖的代码使用 GNU GPL 第三版。第三方组件保留各自许可证。
见 [COPYING](COPYING)、[LICENSE.txt](LICENSE.txt)、[NOTICE](NOTICE)
及[依赖说明](doc/elseport/DEPENDENCIES.md)。保留原有版权和许可声明。

Elseport 与 Oracle 没有关联、认可或赞助关系。Oracle 和 VirtualBox 是 Oracle
及其关联公司的商标；Mac、macOS 是 Apple Inc. 的商标；Ubuntu 是 Canonical Ltd.
的商标。相关名称仅用于说明来源或兼容环境，不代表认可。

贡献前请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。不要在公开问题、补丁或评论中
披露疑似漏洞，按 [SECURITY.md](SECURITY.md) 私下报告。
