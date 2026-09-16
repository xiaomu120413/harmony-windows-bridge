// This file was split (H4 stage 3) into:
//   avc444_gpu_compositor_internal_types.h  — shared type/constant/function declarations
//   avc444_gpu_compositor_utils.cpp          — avc444-unique free functions + DecodedFrame bodies
//   avc444_hardware_decoder.cpp             — Avc444HardwareDecoder
//   avc444_gpu_renderer.cpp                 — Avc444GpuRenderer (shaders + GL, excl. readback)
//   avc444_gpu_readback.cpp                  — Avc444GpuRenderer::SampleFramebuffer/PixelText
//   avc444_gpu_compositor_state.cpp          — Avc444GpuCompositorImpl::State + outer forwarding
//
// It is no longer compiled (removed from CMakeLists.txt). No rendering logic
// changed — only code location and inline->out-of-line placement.
