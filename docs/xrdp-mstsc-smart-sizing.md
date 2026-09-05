# xrdp MSTSC 显示尺寸说明

源码核对：2026-09-05。本文描述 Windows MSTSC 控制 HarmonyOS 2in1，不是鸿蒙客户端尚未实现的缩放工具栏。

## 当前默认值

`harmony/third_party/xrdp/ohos/ohos_private.h` 和应用侧 `xrdp_ini_builder.cpp` 的最大桌面宽高均为 **0**，表示不施加该项固定尺寸上限。旧文档所写的默认 `1920×1280` 已不适用。

`ohos_desktop_size.c` 保留请求桌面尺寸（显式设置正数上限时先限制），再按本机显示比例计算内容目标矩形并居中。因此桌面尺寸、内容矩形和物理显示尺寸不能混为一谈。

## 连接

```powershell
hdc fport tcp:13390 tcp:3390
mstsc /v:127.0.0.1:13390
```

若希望客户端请求特定初始大小，可使用：

```powershell
mstsc /v:127.0.0.1:13390 /w:1920 /h:1280
```

这里的 1920×1280 是用户指定的示例，不是服务端默认值。

## 可选服务端限制

```ini
[OHOS]
max_desktop_width=1920
max_desktop_height=1280
```

这段是显式限制的配置示例。最终结果需结合请求尺寸、实际配置和 `ohos_desktop_size.c` 的显示比例计算验证。本轮没有重新建立 MSTSC 会话，不能宣称某个桌面/采集尺寸已经真机通过。Windows 客户端本地缩放和协议动态分辨率是两种机制；改变窗口不必然意味着服务端分辨率随之改变。
