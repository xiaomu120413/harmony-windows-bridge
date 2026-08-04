cmake_minimum_required(VERSION 3.5.0)
project(rdp_native_bridge)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(FREERDP_OHOS_SYSROOT_INCLUDE
    ${NATIVERENDER_ROOT_PATH}/../../../../../out/ohos-arm64/sysroot/include
    CACHE PATH "FreeRDP OHOS sysroot include directory")
set(FREERDP_OHOS_SOURCE_ROOT
    ${NATIVERENDER_ROOT_PATH}/../../../../../third_party/FreeRDP
    CACHE PATH "FreeRDP source checkout with OHOS client headers")
set(XRDP_OHOS_SOURCE_ROOT
    ${NATIVERENDER_ROOT_PATH}/../../../../../third_party/xrdp
    CACHE PATH "xrdp source checkout with OHOS backend headers")
set(FREERDP_OHOS_RUNTIME_LIB_DIR
    ${NATIVERENDER_ROOT_PATH}/../../../libs/arm64-v8a
    CACHE PATH "Synced FreeRDP OHOS runtime library directory")

set(FREERDP_REQUIRED_HEADERS
    ${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3/freerdp/freerdp.h
    ${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3/freerdp/client.h
    ${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3/freerdp/settings.h
    ${FREERDP_OHOS_SYSROOT_INCLUDE}/winpr3/winpr/synch.h)
set(FREERDP_OHOS_REQUIRED_HEADERS
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_certificate.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_clipboard.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_display.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_graphics.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_ime.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_input_queue.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_keyboard.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_location.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_pointer.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_rdpgfx.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_session.h
    ${FREERDP_OHOS_SOURCE_ROOT}/client/OHOS/ohos_session_config.h)
set(XRDP_OHOS_REQUIRED_HEADERS
    ${XRDP_OHOS_SOURCE_ROOT}/ohos/xrdp_ohos.h)
set(FREERDP_REQUIRED_RUNTIME_LIBS
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libcjson.so.1
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libcrypto.so.3
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libfreerdp-client3.so
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libfreerdp3.so
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libssl.so.3
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libwinpr3.so
    ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libz.so.1)

set(FREERDP_MISSING_PATHS)
foreach(required_path IN LISTS
        FREERDP_REQUIRED_HEADERS
        FREERDP_OHOS_REQUIRED_HEADERS
        XRDP_OHOS_REQUIRED_HEADERS
        FREERDP_REQUIRED_RUNTIME_LIBS)
    if(NOT EXISTS ${required_path})
        list(APPEND FREERDP_MISSING_PATHS ${required_path})
    endif()
endforeach()

if(FREERDP_MISSING_PATHS)
    message(FATAL_ERROR
        "FreeRDP OHOS artifacts are required for the HAP build. "
        "Run harmony/scripts/wsl/build-freerdp-ohos.sh and "
        "harmony/scripts/windows/sync-freerdp-runtime.ps1 first. "
        "Missing: ${FREERDP_MISSING_PATHS}.")
else()
    include_directories(${FREERDP_OHOS_SYSROOT_INCLUDE}/freerdp3
                        ${FREERDP_OHOS_SYSROOT_INCLUDE}/winpr3
                        ${FREERDP_OHOS_SOURCE_ROOT}
                        ${XRDP_OHOS_SOURCE_ROOT})
endif()

set(OPENSSL_CRYPTO_INCLUDE_DIR ${FREERDP_OHOS_SYSROOT_INCLUDE})
set(OPENSSL_CRYPTO_LIBRARY ${FREERDP_OHOS_RUNTIME_LIB_DIR}/libcrypto.so.3)
if(NOT EXISTS ${OPENSSL_CRYPTO_INCLUDE_DIR}/openssl/evp.h)
    message(FATAL_ERROR
        "OpenSSL headers are required for xrdp TLS material generation. "
        "Missing: ${OPENSSL_CRYPTO_INCLUDE_DIR}/openssl/evp.h.")
endif()
if(NOT EXISTS ${OPENSSL_CRYPTO_LIBRARY})
    message(FATAL_ERROR
        "OpenSSL libcrypto runtime is required for xrdp TLS material generation. "
        "Missing: ${OPENSSL_CRYPTO_LIBRARY}.")
endif()

add_library(entry SHARED
    common/bridge_log.cpp
    channels/audio_diagnostics.cpp
    channels/clipboard_bridge.cpp
    channels/rdpgfx_diagnostics.cpp
    channels/rdpgfx_pipeline.cpp
    common/frame_utils.cpp
    input/xcomponent_axis.cpp
    input/xcomponent_input_registration.cpp
    input/xcomponent_key.cpp
    input/xcomponent_mouse.cpp
    input/xcomponent_touch_gesture.cpp
    napi/camera_permission_bridge.cpp
    napi/clipboard_permission_bridge.cpp
    napi/location_bridge.cpp
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
    napi/permission_request_bridge.cpp
    session/rdp_session_core.cpp
    session/rdp_session_channels.cpp
    session/rdp_display_resize_coordinator.cpp
    session/rdp_session_input.cpp
    surface/avc420_gpu_compositor.cpp
    surface/avc420_gpu_compositor_internal.cpp
    surface/avc444_gpu_compositor.cpp
    surface/avc444_gpu_compositor_internal.cpp
    surface/latest_frame_renderer.cpp
    surface/native_rgba_copy.cpp
    surface/native_window_rgba_painter.cpp
    surface/render_output_owner.cpp
    surface/surface_bridge.cpp
    common/string_utils.cpp
    xrdp/xrdp_ini_builder.cpp
    xrdp/xrdp_runtime_config.cpp
    xrdp/xrdp_runtime_loader.cpp
    xrdp/xrdp_server_bridge.cpp)
target_compile_features(entry PUBLIC cxx_std_17)
target_compile_definitions(entry PRIVATE HARMONY_HAS_OPENSSL_CRYPTO=1)
target_include_directories(entry PRIVATE ${OPENSSL_CRYPTO_INCLUDE_DIR})
target_link_libraries(entry PUBLIC
    ${OPENSSL_CRYPTO_LIBRARY}
    libace_napi.z.so
    libace_ndk.z.so
    libEGL.so
    libGLESv3.so
    libnative_image.so
    libnative_media_codecbase.so
    libnative_media_core.so
    libnative_media_vdec.so
    libnative_window.so
    libnative_buffer.so
    libpasteboard.so
    libudmf.so
    libohprint.so
    libohaudio.so
    libhilog_ndk.z.so
    libdl.so)
