// This file was split (H4 stage 2) into five translation units plus a shared
// internal-types header. The original implementation now lives in:
//   surface/avc420_gpu_compositor_utils.cpp          — avc420-specific utilities
//   surface/avc420_hardware_decoder.cpp              — Avc420HardwareDecoder
//   surface/avc420_native_buffer_renderer.cpp        — Avc420NativeBufferRenderer + shaders
//   surface/avc420_gpu_compositor_state.cpp           — Avc420GpuCompositorImpl::State
//   surface/avc420_gpu_compositor_impl.cpp            — Avc420GpuCompositorImpl forwarding
//   surface/avc420_gpu_compositor_internal_types.h    — shared type definitions
// This shell is intentionally empty; CMakeLists.txt no longer compiles it.
