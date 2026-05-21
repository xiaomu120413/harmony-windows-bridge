cmake_minimum_required(VERSION 3.5.0)
project(rdp_native_bridge)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(FREERDP_OHOS_SYSROOT_INCLUDE
    ${NATIVERENDER_ROOT_PATH}/../../../../../out/ohos-arm64/sysroot/include)
set(FREERDP_OHOS_SOURCE_ROOT
    ${NATIVERENDER_ROOT_PATH}/../../../../../third_party/FreeRDP)
set(XRDP_OHOS_SOURCE_ROOT
    ${NATIVERENDER_ROOT_PATH}/../../../../../third_party/xrdp)
if(EXISTS ${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3/freerdp/freerdp.h)
    include_directories(${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3
                        ${FREERDP_OHOS_SYSROOT_INCLUDE}/winpr3)
    add_compile_definitions(HARMONY_HAS_FREERDP_HEADERS=1)
endif()
if(EXISTS ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_keyboard.h)
    include_directories(${FREERDP_OHOS_SOURCE_ROOT})
    add_compile_definitions(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE=1)
endif()
if(EXISTS ${XRDP_OHOS_SOURCE_ROOT}/ohos/xrdp_ohos.h)
    include_directories(${XRDP_OHOS_SOURCE_ROOT})
    add_compile_definitions(HARMONY_HAS_XRDP_OHOS_SOURCE=1)
endif()

add_library(entry SHARED
    common/bridge_log.cpp
    freerdp/certificate_policy.cpp
    channels/audio_diagnostics.cpp
    channels/clipboard_bridge.cpp
    channels/rdpgfx_diagnostics.cpp
    channels/rdpgfx_pipeline.cpp
    common/frame_utils.cpp
    input/ohos_keyboard_adapter.cpp
    input/xcomponent_input_bridge.cpp
    session/freerdp_session_runner.cpp
    freerdp/freerdp_gdi_bridge.cpp
    freerdp/freerdp_runtime.cpp
    freerdp/graphics_config.cpp
    napi/microphone_permission_bridge.cpp
    napi/module_init.cpp
    napi/native_bridge_context.cpp
    common/net_utils.cpp
    napi/napi_event_sink.cpp
    napi/napi_exports.cpp
    napi/napi_utils.cpp
    common/probe_utils.cpp
    freerdp/rdp_channel_config.cpp
    session/rdp_session_core.cpp
    session/rdp_session_channels.cpp
    session/rdp_session_input.cpp
    surface/avc444_gpu_compositor.cpp
    surface/avc444_gpu_compositor_internal.cpp
    surface/latest_frame_renderer.cpp
    surface/native_rgba_copy.cpp
    surface/native_window_rgba_painter.cpp
    surface/render_output_owner.cpp
    surface/surface_bridge.cpp
    common/string_utils.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_audio_capture_bridge.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_capture_common.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_capture_controller.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_capture_h264.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_capture_h264_encoder.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_capture_raw.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_display_geometry.c
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_frame_submitter.cpp
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/ohos_h264_payload.cpp
    xrdp/xrdp_audio_capture_bridge.cpp
    xrdp/xrdp_display_geometry.cpp
    xrdp/xrdp_input_injector.cpp
    xrdp/xrdp_screen_capture_bridge.cpp
    xrdp/xrdp_server_bridge.cpp
    xrdp/xrdp_surface_h264_capture.cpp)
target_compile_features(entry PUBLIC cxx_std_17)
target_link_libraries(entry PUBLIC
    libace_napi.z.so
    libace_ndk.z.so
    libEGL.so
    libGLESv3.so
    libnative_image.so
    libnative_media_codecbase.so
    libnative_media_core.so
    libnative_media_venc.so
    libnative_media_vdec.so
    libnative_avscreen_capture.so
    libnative_display_manager.so
    libohinput.so
    libnative_window.so
    libnative_buffer.so
    libpasteboard.so
    libudmf.so
    libohaudio.so
    libhilog_ndk.z.so
    libdl.so)
