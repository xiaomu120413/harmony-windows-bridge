# Agent Notes

- 后续新增 HarmonyOS native C++ 逻辑必须放进对应模块，不能继续堆到主入口或 N-API 导出文件里。
- `module_init.cpp` / `napi_exports.cpp` 只负责注册、参数转换和转发；FreeRDP、输入、渲染、剪贴板、rdpgfx、音频等逻辑必须进入各自模块。
- 参考 `docs/ohos-native-cpp-module-guidelines.md` 维护模块边界和文件行数限制。
- 分项目参考已有逻辑：xrdp/HarmonyOS 远控线必须优先对照 xrdp 自身的 server、module ABI、`xup`、`vnc`、`neutrinordp`、`libxrdp` 回调和 xrdp 官方配置；FreeRDP/HarmonyOS 客户端线才参考 FreeRDP 客户端平台实现。不要把 FreeRDP 客户端渲染、输入或会话逻辑当成 xrdp 服务端实现模板。
- xrdp 线不要自编协议逻辑、线程时序、格式转换或状态兜底；缺少 xrdp 侧依据时先补调研和日志，再做 OHOS 适配层改动。
