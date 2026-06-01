# 木枢远程桌面第三方开源与构建来源清单

更新日期：2026-05-23

本文档用于鸿蒙上架/商用交付前的第三方开源材料准备。发布包中的动态库以 `harmony/app/entry/libs/arm64-v8a` 为准，构建来源以 `harmony/scripts/wsl/build-freerdp-ohos.sh` 和 `harmony/out/ohos-arm64/manifest.txt` 为准。

## 发布包内第三方组件

| 组件 | 版本/提交 | 许可证 | 发布包文件 | 来源与构建 |
| --- | --- | --- | --- | --- |
| FreeRDP / WinPR | 当前源码树 `harmony/third_party/FreeRDP`，以主仓库 submodule 指针和 `harmony/out/ohos-arm64/manifest.txt` 为准 | Apache-2.0 | `libfreerdp3.so`, `libfreerdp-client3.so`, `libwinpr3.so`, `libfreerdp_ohos_probe.so` | 本仓库子模块源码，脚本 `harmony/scripts/wsl/build-freerdp-ohos.sh` 交叉编译 |
| OpenSSL | 3.3.2 | Apache-2.0 | `libssl.so.3`, `libcrypto.so.3`, `ossl-modules/legacy.so` | 脚本从 OpenSSL 官方 source tarball 或 GitHub release 下载并交叉编译 |
| FFmpeg | 6.1.1 | 当前脚本未开启 `--enable-gpl` / `--enable-nonfree`，按 LGPL 组件交付；最终以 `ffmpeg-configure.log` 输出为准 | `libavcodec.so.60`, `libavdevice.so.60`, `libavfilter.so.9`, `libavformat.so.60`, `libavutil.so.58`, `libswresample.so.4`, `libswscale.so.7` | 脚本从 `ffmpeg.org/releases/ffmpeg-6.1.1.tar.xz` 下载；配置为 shared、no programs、no docs、no debug、enable zlib |
| OpenH264 | 2.4.1 | BSD-2-Clause | `libopenh264.so.7` | 脚本从 Cisco OpenH264 GitHub tag `v2.4.1` 下载并交叉编译 |
| zlib | 1.3.1 | zlib License | `libz.so.1` | 脚本从 zlib 官方 tarball 下载并交叉编译 |
| cJSON | 1.7.18 | MIT | `libcjson.so.1` | 脚本从 DaveGamble/cJSON GitHub tag `v1.7.18` 下载并交叉编译 |
| uriparser | 0.9.8 | BSD-3-Clause | `liburiparser.so.1` | 脚本从 uriparser GitHub release 下载并交叉编译 |
| LLVM libc++ runtime | OHOS SDK 随附版本 | Apache-2.0 WITH LLVM-exception | `libc++_shared.so` | 来自 OpenHarmony/OHOS NDK runtime，随 native 依赖打包 |

## 当前交付 Profile

- 架构：`arm64-v8a`。
- 已关闭：`smartcard`, `PCSC`, `smartcard-pcsc`, `TSMF`, `CUPS`, `FUSE`。
- 已启用：FreeRDP core、TLS/NLA、OpenSSL、zlib、cJSON、uriparser、RDPGFX、geometry dynamic channel、FFmpeg、OpenH264、OHOS AVCodec、OHOS Pasteboard、OHAudio、OHOS LocationKit location backend、固定 Download 目录 `rdpdr/drive` redirection、OHOS PrintKit printer backend；OpenSLES 兼容路径随 OHOS SDK 能力启用。
- FFmpeg 当前 configure 关键项：`--enable-shared`, `--disable-static`, `--disable-programs`, `--disable-doc`, `--disable-debug`, `--disable-autodetect`, `--enable-zlib`，未开启 GPL/nonfree 开关。

## 发布包必须附带的材料

1. 第三方开源 NOTICE 文件：包含本表所有组件、版权主体、许可证类型、版本、源码来源 URL、修改说明。
2. 完整许可证文本：Apache-2.0、OpenSSL Apache-2.0、LGPL-2.1-or-later、BSD-2-Clause、BSD-3-Clause、MIT、zlib License、Apache-2.0 WITH LLVM-exception。
3. 源码/构建来源说明：附 `harmony/scripts/wsl/build-freerdp-ohos.sh`、`harmony/out/ohos-arm64/manifest.txt`、各组件 source tarball URL、构建日志目录 `harmony/out/ohos-arm64/logs`。
4. 修改说明：FreeRDP OHOS client helper、证书策略、剪贴板、音频、geometry 动态通道、地理位置、固定 Download 目录文件重定向、打印、RDPGFX/AVC444 GPU compositor、smartcard/TSMF 裁剪都要列入差异说明。
5. LGPL 履约材料：FFmpeg 如按 LGPL 动态链接交付，需要提供可替换/可重新链接说明、对应源码或源码获取方式、构建脚本与编译参数。
6. OpenH264 专项确认：确认当前以源码自编译方式分发 `libopenh264.so.7` 的专利授权和商用分发边界；若无法确认，应切换为不打包 OpenH264 或改用明确可分发方案。

## 验收点

- `harmony/out/ohos-arm64/manifest.txt` 中 `with_smartcard=OFF`、`with_pcsc=OFF`、`with_smartcard_pcsc=OFF`，发布包中不存在 `smartcard`/`tsmf` 相关动态库。
- `harmony/app/entry/libs/arm64-v8a` 中每个第三方 `.so` 都能在上表找到组件、版本、许可证和来源。
- FFmpeg 构建日志中不存在 `--enable-gpl`、`--enable-nonfree`，并保留 `ffmpeg-configure.log` 供审核。
- NOTICE、license 文本、源码来源说明随最终交付包一起归档。
- 法务/合规对 FFmpeg LGPL 动态链接、OpenH264 专利/分发、OpenSSL 加密合规完成签字确认。

## 风险

- 当前文档是工程交付清单，不替代法务意见；FFmpeg 和 OpenH264 必须单独确认。
- 如果后续重新启用 GPL/nonfree FFmpeg 选项、静态链接 LGPL 组件、或切换 OpenH264 获取方式，NOTICE 和履约方式必须重新评审。
- 如果发布包切换架构或重新构建，应以新的 `manifest.txt` 和 runtime libs 重新生成本表。
