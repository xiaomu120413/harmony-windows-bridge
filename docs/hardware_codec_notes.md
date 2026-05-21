# 硬件编解码相关概念整理

> 这份笔记把前面提到的 `libx11-dev / libX11-devel`、`YUV`、`NV12`、`AVC444 v1/v2`、`ApplyLuma / ApplyChroma`、shader 转 RGB、`highp`、`average undo`、以及 `PCM` 统一整理成一个文档。主线是：**视频/音频的原始数据是什么，硬件编解码器怎么处理，最后怎么显示或播放。**

---

## 1. 总体理解：硬件编解码在处理什么

硬件编解码通常分成两条线：

```text
视频：原始像素帧，例如 YUV / NV12 / YUV444
      -> 硬件编码器，例如 H.264 / H.265 / AV1
      -> 压缩码流
      -> 硬件解码器
      -> 原始像素帧
      -> shader / 显示系统转成 RGB 显示

音频：原始采样数据，例如 PCM
      -> 音频编码器，例如 AAC / Opus / MP3
      -> 压缩码流
      -> 音频解码器
      -> PCM
      -> 声卡 / 音频设备播放
```

所以可以先记住一句话：

```text
YUV / NV12 / PCM 是“原始数据格式”
H.264 / H.265 / AAC / Opus 是“压缩编码格式”
硬件编解码器负责在原始数据和压缩码流之间转换
```

---

## 2. libx11-dev / libX11-devel 是什么

`libx11-dev` 和 `libX11-devel` 是 **X11 客户端库的开发包**。

它们通常不是编解码格式，也不是硬件编码器本身，而是 Linux 图形栈相关的构建依赖。

常见作用：

```text
提供 X11 头文件，例如 X11/Xlib.h
提供链接库，例如 libX11.so
提供 pkg-config 信息，例如 x11.pc
```

不同发行版包名不一样：

```bash
# Debian / Ubuntu
sudo apt install libx11-dev

# Fedora / RHEL / CentOS
sudo dnf install libX11-devel

# openSUSE
sudo zypper install libX11-devel
```

常见报错：

```text
fatal error: X11/Xlib.h: No such file or directory
cannot find -lX11
```

在硬件编解码项目里，它可能出现在这些场景：

```text
视频解码后要开窗口显示
使用 X11 创建窗口、上下文、surface
编译播放器、远程桌面客户端、图形测试程序
```

所以它和编解码的关系更像是：

```text
编解码核心：H.264 / H.265 / NV12 / YUV / shader
显示依赖：X11 / Wayland / EGL / OpenGL / Vulkan
构建依赖：libx11-dev / libX11-devel
```

---

## 3. YUV 分别是什么意思

`YUV` 是一种把图像拆成亮度和颜色的表示方式。

```text
Y = 亮度 / 灰度信息
U = 色度信息之一，偏“蓝色差”
V = 色度信息之一，偏“红色差”
```

更直观一点：

```text
Y：这个像素有多亮
U：这个像素偏不偏蓝
V：这个像素偏不偏红
```

严格来说，数字视频里很多时候说的 `YUV` 实际是 `YCbCr`：

```text
Y  = luma，亮度分量
Cb = blue-difference chroma，蓝色色差，常被叫 U
Cr = red-difference chroma，红色色差，常被叫 V
```

可以粗略理解成：

```text
U / Cb ≈ B - Y
V / Cr ≈ R - Y
```

转 RGB 时的大概关系是：

```text
R = Y + 一部分 V
G = Y - 一部分 U - 一部分 V
B = Y + 一部分 U
```

所以：

```text
Y 控制整体明暗、轮廓、文字细节
U 主要影响蓝/黄方向
V 主要影响红/绿方向
```

如果 U 和 V 搞反，常见现象是：

```text
红蓝互换
肤色发紫或发绿
整体颜色严重不对
```

---

## 4. YUV444 / YUV422 / YUV420 是什么

人眼对亮度细节更敏感，对颜色细节没那么敏感，所以视频里经常完整保留 `Y`，但压缩或降采样 `U/V`。

这就是所谓的 chroma subsampling，也就是色度降采样。

### 4.1 YUV444

```text
每个像素都有自己的 Y/U/V
```

例如 2x2 像素：

```text
Y: y0 y1    U: u0 u1    V: v0 v1
   y2 y3       u2 u3       v2 v3
```

特点：

```text
颜色信息完整
文字、UI、彩色边缘更清晰
数据量更大
```

### 4.2 YUV422

```text
横向每 2 个像素共享一组 U/V
```

特点：

```text
亮度完整
水平色度减半
常见于部分采集、广播、专业视频场景
```

### 4.3 YUV420

```text
每 2x2 个像素共享一组 U/V
```

例如：

```text
Y Y
Y Y

这 4 个 Y 像素共享同一组 U/V
```

特点：

```text
视频编码最常见
数据量小
适合自然视频
对桌面文字、彩色 UI 边缘不如 YUV444 清晰
```

---

## 5. NV12 是什么

`NV12` 是一种非常常见的 **YUV 4:2:0 像素格式**。

它经常出现在：

```text
硬件视频解码输出
硬件视频编码输入
摄像头采集
GPU texture
视频渲染管线
```

NV12 的布局是：

```text
Y plane:  单独一块，完整分辨率
UV plane: U/V 交错存放，分辨率是 Y 的一半
```

也叫：

```text
semi-planar YUV420
半平面 YUV420
```

---

## 6. NV12 内存布局

假设图像是 `width x height`，NV12 的内存通常是：

```text
YYYYYYYYYYYY....
UVUVUVUV....
```

例如一张 `4x4` 图像：

```text
Y plane: 4x4

Y00 Y01 Y02 Y03
Y10 Y11 Y12 Y13
Y20 Y21 Y22 Y23
Y30 Y31 Y32 Y33
```

因为 NV12 是 YUV420，所以每个 `2x2` 像素共享一组 U/V：

```text
UV plane: 2x2 个 UV pair

U00 V00   U01 V01
U10 V10   U11 V11
```

实际内存大概是：

```text
Y00 Y01 Y02 Y03
Y10 Y11 Y12 Y13
Y20 Y21 Y22 Y23
Y30 Y31 Y32 Y33
U00 V00 U01 V01
U10 V10 U11 V11
```

大小通常是：

```text
width * height        // Y
+ width * height / 2  // UV
= width * height * 1.5
```

例如 `1920x1080` 的 8-bit NV12：

```text
1920 * 1080 * 1.5 = 3,110,400 bytes
```

---

## 7. NV12、NV21、I420、YV12 的区别

| 格式 | 类型 | 内存布局 |
|---|---|---|
| `I420` / `YUV420P` | planar | `YYYY... UUUU... VVVV...` |
| `YV12` | planar | `YYYY... VVVV... UUUU...` |
| `NV12` | semi-planar | `YYYY... UVUV...` |
| `NV21` | semi-planar | `YYYY... VUVU...` |

关键区别：

```text
NV12: U V U V U V
NV21: V U V U V U
```

如果 NV12 和 NV21 搞反，常见现象：

```text
颜色严重偏
红蓝不对
肤色变紫或变绿
```

---

## 8. shader 里怎么处理 NV12

在 GPU 里，NV12 常常被拆成两个 texture：

```text
Y texture:  R 通道存 Y
UV texture: R 通道存 U，G 通道存 V
```

fragment shader 里大概会这样：

```glsl
y  = sample(Y_texture).r;
uv = sample(UV_texture).rg;
u  = uv.r;
v  = uv.g;
rgb = YUV_to_RGB(y, u, v);
```

需要注意：

```text
NV12 只说明内存布局
不自动说明颜色矩阵
不自动说明 limited range / full range
```

实际转 RGB 时还要知道：

```text
BT.601 / BT.709 / BT.2020
limited range / full range
```

如果矩阵或 range 用错，可能出现：

```text
画面发灰
黑位不对
白位不对
整体偏色
对比度不正常
```

---

## 9. AVC444 v1/v2 是什么

`AVC444` 可以理解成一种用 H.264/AVC 管线传输或压缩 YUV444 内容的方式。

普通 H.264 硬件编解码器最常见、最兼容的是 YUV420，而桌面、远程桌面、文字 UI 又希望保留更清晰的颜色边缘，也就是更接近 YUV444。

于是 AVC444 的思路是：

```text
把一个 YUV444 帧拆成两个 YUV420 流
再在 decoder 端合回来
```

大概可以理解成：

```text
main / luma view       保存完整亮度 Y，以及一部分基础 chroma
auxiliary / chroma view 保存缺失的 chroma 信息
```

最后组合成：

```text
YUV444
```

再转成：

```text
RGB
```

---

## 10. ApplyLuma 是干什么的

`ApplyLuma()` 主要处理 AVC444 里的 main view，也就是 luma view。

它大概做这些事：

```text
更新 Y plane
写入 main view 里的基础 U/V
为后续 chroma 重建做准备
```

可以理解成：

```text
ApplyLuma = 先把亮度和基础颜色放到目标帧里
```

因为 main view 本质上还是一个 YUV420 图像，所以它的 Y 是完整分辨率，但 U/V 是降采样的。

例如 2x2 像素：

```text
Y:
y0 y1
y2 y3

U/V:
这 4 个像素共享一组 U/V
```

如果只有 `ApplyLuma()`，没有后面的 `ApplyChroma()`，那画面就更像普通 AVC420：

```text
亮度细节在
颜色细节不完整
彩色文字、UI 边缘可能发糊或有彩边
```

---

## 11. ApplyChromaV1 / ApplyChromaV2 是干什么的

`ApplyChromaV1()` 和 `ApplyChromaV2()` 处理 auxiliary / chroma view。

它们的任务是：

```text
按 AVC444 v1 或 v2 的布局
从 auxiliary view 里取出缺失的 chroma 样本
填回 U/V plane
把 YUV420 还原成接近 YUV444
```

v1 和 v2 的区别主要是：

```text
auxiliary view 里 chroma 数据的摆放布局不同
```

所以代码里需要两个函数：

```text
ApplyChromaV1() 按 AVC444 v1 的布局解包
ApplyChromaV2() 按 AVC444 v2 的布局解包
```

如果 v1/v2 布局用错，常见现象：

```text
颜色样本写到错误位置
彩色边缘错位
文字边缘出现明显彩边
画面可能有棋盘状色差
```

---

## 12. AVC444 的 2x2 chroma 还原例子

假设原始 YUV444 的一个 2x2 块是：

```text
Y:
y0 y1
y2 y3

U:
u0 u1
u2 u3

V:
v0 v1
v2 v3
```

YUV444 的特点是：

```text
每个像素都有自己的 U/V
```

但普通 YUV420 对这个 2x2 块只有一组 U/V。

AVC444 的做法可以粗略理解成：

```text
main view 提供完整 Y，以及一个 filtered / averaged chroma
auxiliary view 提供剩下的 chroma 信息
```

例如：

```text
main view:
Y: y0 y1 y2 y3
U: u_filtered
V: v_filtered

auxiliary view:
U: u1 u2 u3
V: v1 v2 v3
```

然后 decoder 端要把它们恢复成：

```text
U:
u0 u1
u2 u3

V:
v0 v1
v2 v3
```

这就是 `ApplyChromaV1()` / `ApplyChromaV2()` 和 `average undo` 要配合完成的事情。

---

## 13. average undo 是干什么的

`average undo` 可以理解成：

```text
把 encoder 端做过的 2x2 chroma 平均反推回来
```

假设 encoder 端把 2x2 chroma 做成了平均值：

```text
u_filtered = (u0 + u1 + u2 + u3) / 4
v_filtered = (v0 + v1 + v2 + v3) / 4
```

decoder 端如果已经从 auxiliary view 里拿到了：

```text
u1, u2, u3
v1, v2, v3
```

那就可以反推缺失的：

```text
u0 = 4 * u_filtered - u1 - u2 - u3
v0 = 4 * v_filtered - v1 - v2 - v3
```

这就是所谓的：

```text
average undo
reverse filter
反平均
反滤波
```

它不是普通意义上的画质增强，也不是锐化，而是为了把 AVC444 编码时为了兼容 YUV420 而做的平均处理尽量还原。

如果没有做 `average undo`，常见现象：

```text
大部分颜色看起来差不多
但彩色文字边缘、细线、UI 边缘会有彩边或发虚
某些红色/蓝色边缘特别明显
```

---

## 14. 最后 shader 把 YUV444 转 RGB 是干什么的

显示器和普通 framebuffer 最终通常需要 RGB，而解码出来的是 YUV。

所以最后 shader 要做：

```text
读取 Y/U/V
必要时做 average undo / reverse filter
把 YUV444 转成 RGB
输出到 framebuffer
```

整体流程可以这样看：

```text
H.264 decode main YUV420
        |
        v
ApplyLuma()
        |
        v
H.264 decode auxiliary YUV420
        |
        v
ApplyChromaV1() / ApplyChromaV2()
        |
        v
还原 YUV444
        |
        v
fragment shader: YUV444 -> RGB
        |
        v
屏幕显示
```

---

## 15. shader 里 highp 是干什么的

`highp` 是 GLSL / OpenGL ES 里的精度限定符，意思是使用较高精度的浮点计算。

在 YUV 转 RGB、AVC444 chroma 还原时，shader 里可能有这些计算：

```text
4 * avg - u1 - u2 - u3
YUV -> RGB matrix
range conversion
clamp
rounding
```

这些计算对精度比较敏感。

如果用 `mediump`，在某些移动端 GPU 或 OpenGL ES 环境里可能精度不够，导致：

```text
轻微偏色
banding
彩边
颜色反推误差
细节边缘异常
```

所以这里用 `highp` 的目的通常是：

```text
减少 chroma 反推误差
减少 YUV -> RGB 的矩阵计算误差
避免颜色边缘因为精度不足而出问题
```

---

## 16. PCM 是什么

`PCM` 全称是：

```text
Pulse-Code Modulation
脉冲编码调制
```

在实际音频开发里，PCM 通常表示：

```text
未压缩的原始音频采样数据
```

它和视频里的 YUV / NV12 很像：

```text
YUV / NV12 = 原始视频帧
PCM        = 原始音频帧 / 原始音频采样
```

而 AAC、Opus、MP3 这类才是压缩音频编码：

```text
PCM -> AAC / Opus / MP3 = 音频编码
AAC / Opus / MP3 -> PCM = 音频解码
```

---

## 17. PCM 的关键参数

看 PCM 不能只看一个 `PCM` 名字，还要看这些参数：

```text
sample rate  采样率
channels     声道数
bit depth    位深
sample format 采样格式
endianness   大小端
interleaved / planar 交错或平面
```

### 17.1 采样率 sample rate

常见采样率：

```text
44100 Hz：音乐常见
48000 Hz：视频、游戏、会议、系统音频常见
96000 Hz：更高规格音频
```

`48000 Hz` 的意思是：

```text
每秒每个声道采样 48000 次
```

### 17.2 声道数 channels

常见声道：

```text
1 channel  = mono，单声道
2 channels = stereo，双声道
6 channels = 5.1
8 channels = 7.1
```

### 17.3 位深 bit depth

常见位深：

```text
16-bit
24-bit
32-bit float
```

位深越高，单个采样占用空间越大，动态范围也更大。

### 17.4 常见 PCM 格式

| 格式 | 含义 |
|---|---|
| `S16LE` | signed 16-bit little-endian |
| `S24LE` | signed 24-bit little-endian |
| `S32LE` | signed 32-bit little-endian |
| `F32LE` | 32-bit float little-endian |
| `U8` | unsigned 8-bit |

`S16LE` 很常见，可以拆开看：

```text
S  = signed，有符号
16 = 每个 sample 16 bit
LE = little-endian，小端
```

---

## 18. PCM 的 interleaved 和 planar

以双声道 stereo 为例，左右声道分别是：

```text
L0 L1 L2 L3 ...
R0 R1 R2 R3 ...
```

### 18.1 interleaved PCM

交错存储：

```text
L0 R0 L1 R1 L2 R2 L3 R3 ...
```

很多音频设备、WAV 文件、系统音频 API 常用这种。

### 18.2 planar PCM

平面存储：

```text
LLLLLLLL....
RRRRRRRR....
```

一些音频处理库、编解码库内部可能使用 planar。

如果 interleaved / planar 理解错，声音会异常：

```text
声道错乱
噪音
播放速度或节奏不正常
左右声道不对
```

---

## 19. PCM 数据量怎么算

公式：

```text
bytes_per_second = sample_rate * channels * bits_per_sample / 8
```

例如：

```text
48000 Hz
2 channels
16-bit
```

那么：

```text
48000 * 2 * 16 / 8 = 192000 bytes/s
```

也就是：

```text
约 187.5 KiB/s
约 1.536 Mbps
```

如果是 10 秒音频：

```text
192000 * 10 = 1,920,000 bytes
```

这还是未压缩数据，所以 PCM 比 AAC / Opus 这类压缩音频要大很多。

---

## 20. PCM 和硬件音频编解码的关系

音频采集和播放通常围绕 PCM：

```text
麦克风采集 -> PCM
PCM -> 音频编码器 -> AAC / Opus / MP3
AAC / Opus / MP3 -> 音频解码器 -> PCM
PCM -> 声卡播放
```

所以音频方向可以类比视频方向：

| 视频 | 音频 |
|---|---|
| YUV / NV12 | PCM |
| H.264 / H.265 / AV1 | AAC / Opus / MP3 |
| 视频编码器 | 音频编码器 |
| 视频解码器 | 音频解码器 |
| shader / 显示器 | audio device / speaker |

---

## 21. 一个完整的音视频硬件编解码流程

### 21.1 编码端

```text
屏幕 / 摄像头采集
        |
        v
原始视频帧：RGB / YUV / NV12
        |
        v
颜色转换 / 格式转换，例如 RGB -> NV12
        |
        v
硬件视频编码器：H.264 / H.265 / AV1
        |
        v
视频压缩码流
```

同时音频：

```text
麦克风 / 系统声音采集
        |
        v
PCM
        |
        v
重采样 / 混音 / 声道转换
        |
        v
音频编码器：AAC / Opus
        |
        v
音频压缩码流
```

然后：

```text
音频码流 + 视频码流
        |
        v
封装 / 传输，例如 MP4 / MKV / RTP / WebRTC / RDP
```

### 21.2 解码端

```text
接收 / 读取压缩码流
        |
        v
硬件视频解码器
        |
        v
NV12 / YUV420 / YUV444
        |
        v
shader 转 RGB
        |
        v
显示
```

音频：

```text
接收 / 读取音频码流
        |
        v
音频解码器
        |
        v
PCM
        |
        v
音频设备播放
```

---

## 22. 把前面几个概念放到同一张表里

| 概念 | 类型 | 主要作用 |
|---|---|---|
| `YUV` | 原始视频颜色表示 | 把图像拆成亮度 Y 和色度 U/V |
| `NV12` | 原始视频像素格式 | Y 单独存，UV 交错存，YUV420 常见格式 |
| `YUV444` | 原始视频格式 | 每个像素都有完整 Y/U/V，颜色细节完整 |
| `YUV420` | 原始视频格式 | 每 2x2 像素共享 U/V，视频编码常见 |
| `AVC444` | 视频编码/传输方案 | 用两个 YUV420 流组合还原 YUV444 |
| `ApplyLuma` | 解码后处理函数 | 更新 Y plane 和基础 chroma |
| `ApplyChromaV1/V2` | 解码后处理函数 | 按 AVC444 v1/v2 布局还原 chroma |
| `average undo` | chroma 反滤波 | 从平均 chroma 反推缺失样本 |
| `shader YUV->RGB` | 显示阶段 | 把 YUV 转成屏幕需要的 RGB |
| `highp` | shader 精度控制 | 减少颜色计算和反推误差 |
| `PCM` | 原始音频格式 | 未压缩音频采样数据 |
| `libx11-dev` / `libX11-devel` | 构建依赖 | 提供 X11 图形库头文件和链接库 |

---

## 23. 常见问题和排查方向

### 23.1 视频颜色不对

可能原因：

```text
NV12 / NV21 搞反
U/V 顺序错
BT.601 / BT.709 用错
limited range / full range 用错
shader 采样通道错
```

表现：

```text
肤色发紫或发绿
红蓝互换
整体偏色
画面发灰或过曝
```

### 23.2 视频边缘有彩边

可能原因：

```text
YUV420 色度降采样导致
AVC444 chroma 没还原完整
ApplyChromaV1/V2 布局用错
average undo 没做或公式不对
shader 精度不够，例如 mediump 误差
```

表现：

```text
彩色文字边缘发毛
红色/蓝色细线边缘异常
UI 边缘出现颜色错位
```

### 23.3 画面花屏或错位

可能原因：

```text
stride / pitch 当成 width 用
plane offset 算错
Y plane 和 UV plane 起始地址错
纹理尺寸或采样坐标不对
硬件 decoder 输出有对齐要求但没有处理
```

### 23.4 音频有噪音

可能原因：

```text
PCM 格式理解错，例如 S16LE 当成 F32LE
大小端错
采样率错
声道数错
interleaved / planar 搞反
buffer size 或 frame size 算错
```

### 23.5 音频变快或变慢

可能原因：

```text
实际采样率和播放采样率不一致
例如 44100 Hz 的 PCM 按 48000 Hz 播放
```

---

## 24. 一句话总结

```text
YUV / NV12 是视频原始帧格式
PCM 是音频原始采样格式
H.264 / H.265 / AAC / Opus 是压缩编码格式
AVC444 是为了用常见 H.264/YUV420 管线还原更清晰的 YUV444
ApplyLuma / ApplyChroma 是 AVC444 解包和 chroma 重建
shader 负责最终把 YUV 转 RGB 显示
libx11-dev / libX11-devel 是 Linux 图形显示相关的开发依赖，不是编解码格式本身
```

最终可以用这条主线记：

```text
视频：NV12 / YUV -> 编码器 -> 码流 -> 解码器 -> YUV/NV12 -> shader -> RGB -> 屏幕
音频：PCM -> 编码器 -> 码流 -> 解码器 -> PCM -> 声卡 -> 声音
```
