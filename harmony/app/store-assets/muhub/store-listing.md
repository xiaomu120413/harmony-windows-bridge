# MuHub Store Listing

## 简体中文

应用名称：木枢

英文名称：MuHub

Bundle Name：com.muhub.desktop

一句话简介：鸿蒙与Windows互联

应用介绍：

木枢（MuHub）是一款面向 HarmonyOS 与 Windows 的互联工具，帮助用户在本地网络中完成桌面远控、键鼠操作、剪贴板协同与设备间内容流转。你可以在 Windows 上使用系统自带的远程桌面连接 HarmonyOS 设备，也可以从 HarmonyOS 侧连接 Windows 桌面，在办公、调试、演示和多设备协作场景中更高效地切换设备。

主要能力：

1. Windows 远程控制 HarmonyOS 设备
支持使用 Windows 自带远程桌面连接工具访问 HarmonyOS 设备画面，完成鼠标、键盘、滚轮等基础操作。

2. HarmonyOS 连接 Windows 桌面
支持从 HarmonyOS 设备发起远程桌面连接，查看并操作 Windows 桌面。

3. 剪贴板与内容协同
支持文本、图片等常用内容在 HarmonyOS 与 Windows 之间流转，减少重复传输和手动保存步骤。

4. 本地网络互联
面向局域网和可信网络环境使用，连接过程由用户主动发起，关键能力需要系统授权后才会启用。

5. 面向桌面协作的体验
针对远控画面、输入映射、分辨率同步和连接诊断做了适配，适合办公协作、远程调试、设备演示和跨设备操作。

权限与安全说明：

木枢会根据功能使用需要申请屏幕录制、输入控制、网络访问、剪贴板或文件访问等权限。屏幕录制用于将设备画面发送到已连接的远程桌面客户端；输入控制用于响应用户在远程端执行的鼠标和键盘操作；网络访问用于在本地网络中建立连接；剪贴板或文件访问用于完成用户主动触发的内容协同。相关能力均需用户授权或主动操作后启用。

适用场景：

本地办公中临时控制另一台设备；开发调试时查看 HarmonyOS 设备画面；会议演示时将设备操作投到 Windows；在 HarmonyOS 与 Windows 之间复制文字、图片和文件；需要通过 Windows 自带远程桌面工具访问 HarmonyOS 设备。

## English

App Name: MuHub

Chinese Name: 木枢

Bundle Name: com.muhub.desktop

Short Description: HarmonyOS-Windows bridge

Description:

MuHub is an interconnectivity tool for HarmonyOS and Windows. It helps users access remote desktops, control devices with keyboard and mouse, and move common content such as text and images across devices on a trusted local network.

MuHub supports Windows Remote Desktop access to a HarmonyOS device and HarmonyOS-side remote desktop access to Windows. It is designed for local office workflows, development debugging, device demonstrations, and daily cross-device collaboration.

Key capabilities:

1. Remote control from Windows to HarmonyOS with the built-in Windows Remote Desktop client.
2. Remote desktop access from HarmonyOS to Windows.
3. Keyboard, mouse, wheel, clipboard, image, and content collaboration features.
4. Local-network oriented connection flow with user-controlled permissions.
5. Desktop-focused display, input mapping, resolution handling, and diagnostics.

Permissions:

MuHub may request screen recording, input control, network access, clipboard access, or file access depending on the feature being used. Screen recording is used to send device frames to the connected remote client. Input control is used to handle remote keyboard and mouse operations. Network access is used to establish local connections. Clipboard and file access are used for user-initiated content collaboration.

