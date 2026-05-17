# Agent Notes

- 后续新增 HarmonyOS native C++ 逻辑必须放进对应模块，不能继续堆到主入口或 N-API 导出文件里。
- `module_init.cpp` / `napi_exports.cpp` 只负责注册、参数转换和转发；FreeRDP、输入、渲染、剪贴板、rdpgfx、音频等逻辑必须进入各自模块。
- 参考 `docs/ohos-native-cpp-module-guidelines.md` 维护模块边界和文件行数限制。
