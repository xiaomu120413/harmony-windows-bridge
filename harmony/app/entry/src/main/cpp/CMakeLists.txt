cmake_minimum_required(VERSION 3.5.0)
project(xrdp_process_control)

set(CONTROL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(OHOS_SYSROOT_INCLUDE ${CONTROL_ROOT}/../../../../../out/ohos-arm64/sysroot/include)
set(COMMON_RUNTIME_LIB_DIR ${CONTROL_ROOT}/../../../../common/libs/arm64-v8a)
set(OPENSSL_CRYPTO_LIBRARY ${COMMON_RUNTIME_LIB_DIR}/libcrypto.so.3)

if(NOT EXISTS ${OHOS_SYSROOT_INCLUDE}/openssl/evp.h OR
   NOT EXISTS ${OPENSSL_CRYPTO_LIBRARY})
    message(FATAL_ERROR "OpenSSL headers and libcrypto.so.3 are required for xrdp TLS setup")
endif()

add_library(xrdpcontrol SHARED
    napi/xrdpcontrol_napi.cpp
    xrdp/xrdp_ini_builder.cpp
    xrdp/xrdp_runtime_config.cpp
    xrdp/xrdp_runtime_loader.cpp
    xrdp/xrdp_server_bridge.cpp)
target_compile_features(xrdpcontrol PUBLIC cxx_std_17)
target_compile_definitions(xrdpcontrol PRIVATE HARMONY_HAS_OPENSSL_CRYPTO=1)
target_include_directories(xrdpcontrol PRIVATE ${CONTROL_ROOT} ${OHOS_SYSROOT_INCLUDE})
target_link_libraries(xrdpcontrol PUBLIC
    ${OPENSSL_CRYPTO_LIBRARY}
    libace_napi.z.so
    libhilog_ndk.z.so)
