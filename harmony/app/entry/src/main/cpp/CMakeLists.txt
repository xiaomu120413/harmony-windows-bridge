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

add_library(entry SHARED napi_init.cpp)
target_compile_features(entry PUBLIC cxx_std_17)
target_link_libraries(entry PUBLIC
    libace_napi.z.so
    libace_ndk.z.so
    libnative_window.so
    libnative_buffer.so
    libpasteboard.so
    libudmf.so
    libdl.so)
