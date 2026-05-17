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
if(EXISTS ${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3/freerdp/freerdp.h)
    include_directories(${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3
                        ${FREERDP_OHOS_SYSROOT_INCLUDE}/winpr3)
    add_compile_definitions(HARMONY_HAS_FREERDP_HEADERS=1)
endif()

add_library(entry SHARED
    bridge_log.cpp
    certificate_policy.cpp
    channels/audio_diagnostics.cpp
    channels/clipboard_bridge.cpp
    channels/clipboard_format.cpp
    channels/clipboard_pasteboard.cpp
    channels/rdpgfx_diagnostics.cpp
    channels/rdpgfx_pipeline.cpp
    frame_utils.cpp
    freerdp_session_runner.cpp
    freerdp_gdi_bridge.cpp
    freerdp_runtime.cpp
    graphics_config.cpp
    module_init.cpp
    native_bridge_context.cpp
    net_utils.cpp
    napi_event_sink.cpp
    napi_exports.cpp
    napi_utils.cpp
    probe_utils.cpp
    rdp_channel_config.cpp
    rdp_session_core.cpp
    rdp_session_channels.cpp
    rdp_session_input.cpp
    surface/latest_frame_renderer.cpp
    surface/native_rgba_copy.cpp
    surface/native_window_rgba_painter.cpp
    surface/surface_bridge.cpp
    string_utils.cpp)
target_compile_features(entry PUBLIC cxx_std_17)
target_link_libraries(entry PUBLIC
    libace_napi.z.so
    libace_ndk.z.so
    libEGL.so
    libGLESv3.so
    libnative_image.so
    libnative_window.so
    libnative_buffer.so
    libpasteboard.so
    libudmf.so
    libohaudio.so
    libhilog_ndk.z.so
    libdl.so)
