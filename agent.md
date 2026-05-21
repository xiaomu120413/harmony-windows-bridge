# Agent Notes

- 后续新增 HarmonyOS native C++ 逻辑必须放进对应模块，不能继续堆到主入口或 N-API 导出文件里。
- `module_init.cpp` / `napi_exports.cpp` 只负责注册、参数转换和转发；FreeRDP、输入、渲染、剪贴板、rdpgfx、音频等逻辑必须进入各自模块。
- 参考 `docs/ohos-native-cpp-module-guidelines.md` 维护模块边界和文件行数限制。
- xrdp/HarmonyOS 远控实现必须先对照 xrdp、FreeRDP 已有平台后端或官方协议实现，按既有连接、输入、帧更新、通道、剪贴板等路径适配 OHOS；不要自编协议逻辑、线程时序、格式转换或状态兜底，缺少依据时先补调研和日志。
