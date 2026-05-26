cmake_minimum_required(VERSION 3.16)

project(freerdp_bridge C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_path(FREERDP_INCLUDE_DIR
  NAMES freerdp/freerdp.h
  HINTS "$ENV{FREERDP_ROOT}"
  PATH_SUFFIXES include include/freerdp3
)

find_path(WINPR_INCLUDE_DIR
  NAMES winpr/wtypes.h
  HINTS "$ENV{FREERDP_ROOT}"
  PATH_SUFFIXES include include/winpr3
)

find_library(FREERDP_LIBRARY
  NAMES freerdp3 freerdp2 freerdp
  HINTS "$ENV{FREERDP_ROOT}"
  PATH_SUFFIXES lib
)

find_library(WINPR_LIBRARY
  NAMES winpr3 winpr2 winpr
  HINTS "$ENV{FREERDP_ROOT}"
  PATH_SUFFIXES lib
)

if(NOT FREERDP_INCLUDE_DIR OR NOT WINPR_INCLUDE_DIR OR NOT FREERDP_LIBRARY OR NOT WINPR_LIBRARY)
  message(FATAL_ERROR "FreeRDP development files were not found. Set FREERDP_ROOT to the FreeRDP install prefix.")
endif()

add_executable(freerdp_bridge src/main.c)
target_include_directories(freerdp_bridge PRIVATE "${FREERDP_INCLUDE_DIR}" "${WINPR_INCLUDE_DIR}")
target_link_libraries(freerdp_bridge PRIVATE "${FREERDP_LIBRARY}" "${WINPR_LIBRARY}")

if(MSVC)
  target_compile_options(freerdp_bridge PRIVATE /W4)
else()
  target_compile_options(freerdp_bridge PRIVATE -Wall -Wextra -Wpedantic)
endif()
