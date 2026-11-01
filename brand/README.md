# Elseport 品牌设计 01

状态：用户已选定 Elseport；本目录为原创品牌提案，尚未整合到产品。当前版本用于审阅，不表示完成商标清查或获得发布许可。

## 标记

从一个向右敞开的 E 出发：圆角脊线连起三条横笔，独立圆点穿过右侧开口。它表达“从当前空间到另一处空间”，而不是某一操作系统或硬件架构。

主图形由本项目独立绘制，没有使用第三方图标库、图库或图像生成服务。它不使用立方体或双竖条作为识别结构。简单几何形状仍可能与其他设计相似，本目录不提供商标独占性保证。

| 文件 | 用途 |
| --- | --- |
| `elseport-master.svg` | 128 × 128 坐标系的可编辑图形主文件 |
| `logo-light.svg` | 浅底横向组合，主色标记与深色字标 |
| `logo-dark.svg` | 深底横向组合，浅紫标记与浅色字标 |
| `logo-mono.svg` / `logo-reversed.svg` | 黑色单色 / 白色反白组合 |
| `mark.svg` / `mark-mono.svg` / `mark-reversed.svg` | 独立图形的三种颜色版本 |
| `wordmark.svg` | 已转曲的字标，不依赖使用者安装字体 |
| `app-icon.svg` / `app-icon-1024.png` | 应用图标主文件与 1024 px 导出 |
| `Elseport.icns` / `Elseport.iconset/` | macOS 应用图标与标准多尺寸素材 |
| `menu-template.svg` / `menu-template.png` / `menu-template@2x.png` | 菜单栏模板素材；实际接入时设置 template 属性 |
| `type/Manrope.ttf` / `type/OFL.txt` | 字标字体及其原始许可证 |

## 一页使用规范

- 主色：`#6659D8`。浅底字标：`#1B1C30`。深底字标：`#F5F4FF`。深底标记：`#B5ACFF`。
- 点色：`#C4F0E1`，仅用于彩色应用图标。普通标记中圆点与主体同色，保持小尺寸清楚。
- 安全空间：图形实际轮廓外，至少留出一个圆点半径，即主文件中的 9 单位。横向组合外围也采用这一最小值。
- 最小尺寸：独立标记 20 px，完整字标宽 100 px。16 px 应用图标是系统资源，不应用作正文中的品牌标记。
- 字标使用 Manrope 650，字距已微调，并转为路径。不要重新打字来代替已转曲字标。
- 浅底使用 `logo-light.svg`，深底使用 `logo-dark.svg`；复杂背景用单色或反白版，保证背景对比。
- 不拉伸、不倾斜完整字标、不单独移动圆点、不添加外描边、不随意改渐变。圆点不是通知标记。
- 应用图标包含自己的圆角背景；不要将普通横向字标放入系统图标方框。菜单栏只用单色模板。
- 当前尚无独立中文商标。中文说明语为“一台电脑，更多可能。”，不与英文名等同为已注册商标。

可视规范页：[品牌预览](../doc/design/brand.html)。

## 字体来源与许可

Manrope 来自 [Google Fonts 官方目录](https://github.com/google/fonts/tree/main/ofl/manrope)，设计者为 Mikhail Sharanda。下载日期：2026-10-31。字体按 SIL Open Font License 1.1 分发，原始版权声明与完整许可保留于 `type/OFL.txt`。字标轮廓用于本项目；字体文件自身仍遵循 OFL，不改成项目代码许可证。

- `Manrope.ttf` SHA-256：`3ae11c49db0455a3cc33e37d380f20fdb8c7f8b41dc07625c177e3d87a9d6ae6`
- `OFL.txt` SHA-256：`58172e0c0fac2cda8a37b348164bb55e44b0e69051e557e92b1d3f6910141f7b`

新增图形没有冒用上游版权人署名。项目自身的权利人署名仍按既有内部审查记录处理；不要用字体作者或 Oracle 的名义为本图形背书。

## 重新导出

已有 `rsvg-convert` 和 Apple Command Line Tools 的开发环境中，在仓库根目录运行：

```sh
DEVELOPER_DIR=/Library/Developer/CommandLineTools swift brand/tools/outline-wordmark.swift brand/type/Manrope.ttf brand/wordmark.svg
python3 brand/tools/build-assets.py
```

第二步读取 `elseport-master.svg`，输出颜色变体、PNG、ICNS，以及界面原型所需的本地资源。它只写本目录与 `doc/design/prototype/assets/`，不修改已安装应用，不更改系统设置。
