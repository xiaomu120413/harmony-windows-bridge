#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
FREE_RDP_SRC="$REPO_ROOT/harmony/third_party/FreeRDP"
FREERDP_BUILD_SRC=""

OUT_DIR="${OUT_DIR:-$REPO_ROOT/harmony/out/ohos-arm64}"
PREFIX="$OUT_DIR/sysroot"
LOG_DIR="$OUT_DIR/logs"
RUNTIME_DIR="$OUT_DIR/runtime-libs"

OHOS_ARCH="${OHOS_ARCH:-arm64-v8a}"
OHOS_TRIPLE="${OHOS_TRIPLE:-aarch64-linux-ohos}"
WORK_DIR="${OHOS_BUILD_WORKDIR:-$HOME/.cache/demo-ohos-${OHOS_ARCH}}"
SRC_DIR="$WORK_DIR/src"
BUILD_DIR="$WORK_DIR/build"

OPENSSL_VERSION="${OPENSSL_VERSION:-3.3.2}"
ZLIB_VERSION="${ZLIB_VERSION:-1.3.1}"
CJSON_VERSION="${CJSON_VERSION:-1.7.18}"
URIPARSER_VERSION="${URIPARSER_VERSION:-0.9.8}"
OPENH264_VERSION="${OPENH264_VERSION:-2.4.1}"
FFMPEG_VERSION="${FFMPEG_VERSION:-6.1.1}"
JOBS="${JOBS:-$(nproc)}"
FORCE_REBUILD="${FORCE_REBUILD:-0}"
ENABLE_URIPARSER="${ENABLE_URIPARSER:-1}"
ENABLE_OPENH264="${ENABLE_OPENH264:-1}"
ENABLE_FFMPEG="${ENABLE_FFMPEG:-1}"
ENABLE_OHAUDIO="${ENABLE_OHAUDIO:-1}"
ENABLE_OHOS_AVCODEC="${ENABLE_OHOS_AVCODEC:-1}"
ENABLE_OHOS_PASTEBOARD="${ENABLE_OHOS_PASTEBOARD:-1}"
ENABLE_OHOS_PRINT="${ENABLE_OHOS_PRINT:-1}"
ENABLE_OPENSLES="${ENABLE_OPENSLES:-0}"
ENABLE_CUPS="${ENABLE_CUPS:-0}"
ENABLE_SMARTCARD=0
ENABLE_PCSC=0
ENABLE_SMARTCARD_PCSC=0
ENABLE_FUSE="${ENABLE_FUSE:-0}"
ENABLE_CCACHE="${ENABLE_CCACHE:-auto}"
CCACHE_PROGRAM="${CCACHE_PROGRAM:-ccache}"

WITH_OHAUDIO=OFF
WITH_OHOS_AVCODEC=OFF
WITH_OHOS_PASTEBOARD=OFF
WITH_OHOS_PRINT=OFF
WITH_OPENSLES=OFF
CCACHE_LAUNCHER=""
FREERDP_FEATURE_PROFILE="channels-codecs-ohos-avcodec-pasteboard-print-location-geometry-no-smartcard-tsmf-v1"

log() {
  printf '\n==> %s\n' "$*"
}

require_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'Missing required tool: %s\n' "$1" >&2
    exit 1
  fi
}

is_enabled() {
  case "${1:-0}" in
    1|ON|on|On|true|TRUE|yes|YES) return 0 ;;
    *) return 1 ;;
  esac
}

is_auto() {
  [[ "${1:-}" == "auto" || "${1:-}" == "AUTO" ]]
}

cmake_bool() {
  if is_enabled "$1"; then
    printf 'ON'
  else
    printf 'OFF'
  fi
}

configure_ccache() {
  CCACHE_LAUNCHER=""

  if ! is_enabled "$ENABLE_CCACHE" && ! is_auto "$ENABLE_CCACHE"; then
    log "ccache disabled"
    return 0
  fi

  if command -v "$CCACHE_PROGRAM" >/dev/null 2>&1; then
    CCACHE_LAUNCHER="$(command -v "$CCACHE_PROGRAM")"
    export CCACHE_DIR="${CCACHE_DIR:-$WORK_DIR/ccache}"
    export CCACHE_BASEDIR="${CCACHE_BASEDIR:-$WORK_DIR}"
    export CCACHE_NOHASHDIR="${CCACHE_NOHASHDIR:-true}"
    mkdir -p "$CCACHE_DIR"
    log "ccache enabled: $CCACHE_LAUNCHER"
    return 0
  fi

  if is_enabled "$ENABLE_CCACHE"; then
    printf 'ENABLE_CCACHE requested but ccache was not found: %s\n' "$CCACHE_PROGRAM" >&2
    exit 1
  fi

  log "ccache unavailable; compiler cache disabled"
}

safe_rm_rf() {
  local target="$1"
  [[ -e "$target" ]] || return 0

  local resolved repo_out work_base
  resolved="$(realpath -m "$target")"
  repo_out="$(realpath -m "$REPO_ROOT/harmony/out")"
  work_base="$(realpath -m "$WORK_DIR")"

  case "$resolved" in
    "$repo_out"/*|"$work_base"/*)
      rm -rf "$resolved"
      ;;
    *)
      printf 'Refusing to remove path outside build/output dirs: %s\n' "$resolved" >&2
      exit 1
      ;;
  esac
}

load_ohos_env() {
  if [[ -z "${OHOS_NDK_HOME:-}" && -f /etc/profile.d/ohos-sdk.sh ]]; then
    # shellcheck disable=SC1091
    . /etc/profile.d/ohos-sdk.sh
  fi

  : "${OHOS_NDK_HOME:?Set OHOS_NDK_HOME to the Linux OHOS native SDK path}"
  : "${OHOS_LLVM_HOME:=$OHOS_NDK_HOME/llvm}"

  export PATH="$OHOS_NDK_HOME/build-tools/cmake/bin:$OHOS_LLVM_HOME/bin:/usr/local/bin:/usr/bin:/bin:$PATH"

  [[ -f "$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake" ]] || {
    printf 'OHOS CMake toolchain was not found under %s\n' "$OHOS_NDK_HOME" >&2
    exit 1
  }

  [[ -x "$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" ]] || {
    printf 'OHOS arm64 clang wrapper was not found under %s\n' "$OHOS_LLVM_HOME" >&2
    exit 1
  }
}

download_archive() {
  local output="$1"
  shift

  [[ -f "$output" ]] && return 0

  local tmp="${output}.tmp"
  safe_rm_rf "$tmp"
  for url in "$@"; do
    log "download $(basename "$output")"
    printf '%s\n' "$url"
    if curl -fL --retry 3 --connect-timeout 20 --output "$tmp" "$url"; then
      mv "$tmp" "$output"
      return 0
    fi
  done

  printf 'Failed to download %s\n' "$output" >&2
  exit 1
}

download_tarball() {
  download_archive "$@"
}

extract_archive() {
  local archive="$1"
  local directory="$2"

  [[ -d "$directory" ]] && return 0
  log "extract $(basename "$archive")"
  case "$archive" in
    *.tar.gz|*.tgz)
      tar -xzf "$archive" -C "$SRC_DIR"
      ;;
    *.tar.xz|*.txz)
      tar -xJf "$archive" -C "$SRC_DIR"
      ;;
    *)
      printf 'Unsupported archive type: %s\n' "$archive" >&2
      exit 1
      ;;
  esac
}

extract_tarball() {
  extract_archive "$@"
}

detect_optional_backends() {
  [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/filemanagement/environment/oh_environment.h" ]] || {
    printf 'OHOS Environment header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/filemanagement/environment" >&2
    exit 1
  }
  [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libohenvironment.so" ]] || {
    printf 'OHOS Environment library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
    exit 1
  }

  WITH_OHAUDIO="$(cmake_bool "$ENABLE_OHAUDIO")"
  if [[ "$WITH_OHAUDIO" == "ON" ]]; then
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/ohaudio/native_audiorenderer.h" ]] || {
      printf 'OHAudio header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/ohaudio" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/ohaudio/native_audiocapturer.h" ]] || {
      printf 'OHAudio capturer header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/ohaudio" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libohaudio.so" ]] || {
      printf 'OHAudio library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
  fi

  WITH_OHOS_AVCODEC="$(cmake_bool "$ENABLE_OHOS_AVCODEC")"
  if [[ "$WITH_OHOS_AVCODEC" == "ON" ]]; then
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/multimedia/player_framework/native_avcodec_videodecoder.h" ]] || {
      printf 'AVCodec video decoder header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/multimedia/player_framework" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libnative_media_vdec.so" ]] || {
      printf 'AVCodec video decoder library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libnative_media_codecbase.so" ]] || {
      printf 'AVCodec codecbase library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
  fi

  WITH_OHOS_PASTEBOARD="$(cmake_bool "$ENABLE_OHOS_PASTEBOARD")"
  if [[ "$WITH_OHOS_PASTEBOARD" == "ON" ]]; then
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/database/pasteboard/oh_pasteboard.h" ]] || {
      printf 'Pasteboard header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/database/pasteboard" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/database/udmf/udmf.h" ]] || {
      printf 'UDMF header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/database/udmf" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libpasteboard.so" ]] || {
      printf 'Pasteboard library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libudmf.so" ]] || {
      printf 'UDMF library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
  fi

  WITH_OHOS_PRINT="$(cmake_bool "$ENABLE_OHOS_PRINT")"
  if [[ "$WITH_OHOS_PRINT" == "ON" ]]; then
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/include/BasicServicesKit/ohprint.h" ]] || {
      printf 'OHOS Print header was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/include/BasicServicesKit" >&2
      exit 1
    }
    [[ -f "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libohprint.so" ]] || {
      printf 'OHOS Print library was not found under %s\n' "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE" >&2
      exit 1
    }
  fi

  if is_auto "$ENABLE_OPENSLES"; then
    if find "$OHOS_NDK_HOME" \( -path '*/SLES/OpenSLES.h' -o -name 'libOpenSLES.so' \) -print -quit | grep -q .; then
      WITH_OPENSLES=ON
    else
      WITH_OPENSLES=OFF
    fi
  else
    WITH_OPENSLES="$(cmake_bool "$ENABLE_OPENSLES")"
  fi

  log "optional backends"
  printf 'OHOS_Environment=ON OHAudio=%s OHOS_AVCodec=%s OHOS_Pasteboard=%s OHOS_Print=%s OpenSLES=%s CUPS=%s Smartcard=%s PCSC=%s FUSE=%s\n' \
    "$WITH_OHAUDIO" "$WITH_OHOS_AVCODEC" "$WITH_OHOS_PASTEBOARD" "$WITH_OHOS_PRINT" "$WITH_OPENSLES" "$(cmake_bool "$ENABLE_CUPS")" "$(cmake_bool "$ENABLE_SMARTCARD")" "$(cmake_bool "$ENABLE_PCSC")" "$(cmake_bool "$ENABLE_FUSE")"
}

install_ohos_opensles_android_shim() {
  if [[ "$WITH_OPENSLES" != "ON" ]]; then
    return 0
  fi

  local shim_dir="$PREFIX/include/SLES"
  local shim="$shim_dir/OpenSLES_Android.h"
  mkdir -p "$shim_dir"
  cat > "$shim" <<'EOF'
#ifndef OHOS_FREERDP_OPENSLES_ANDROID_COMPAT_H
#define OHOS_FREERDP_OPENSLES_ANDROID_COMPAT_H

#include <SLES/OpenSLES.h>

/*
 * FreeRDP's OpenSLES backend is written for Android's simple buffer queue API.
 * HarmonyOS exposes standard OpenSL ES buffer queues instead, so this shim lets
 * the existing backend compile for the OHOS NDK while a dedicated AudioRenderer
 * backend is still pending.
 */
typedef SLBufferQueueItf SLAndroidSimpleBufferQueueItf;
typedef SLDataLocator_BufferQueue SLDataLocator_AndroidSimpleBufferQueue;

#define SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE SL_DATALOCATOR_BUFFERQUEUE
#define SL_IID_ANDROIDSIMPLEBUFFERQUEUE SL_IID_BUFFERQUEUE

#endif
EOF
}

prepare_sources() {
  mkdir -p "$SRC_DIR" "$BUILD_DIR" "$PREFIX" "$LOG_DIR"

  download_tarball "$SRC_DIR/openssl-$OPENSSL_VERSION.tar.gz" \
    "https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz" \
    "https://www.openssl.org/source/old/3.3/openssl-$OPENSSL_VERSION.tar.gz" \
    "https://github.com/openssl/openssl/releases/download/openssl-$OPENSSL_VERSION/openssl-$OPENSSL_VERSION.tar.gz"
  extract_tarball "$SRC_DIR/openssl-$OPENSSL_VERSION.tar.gz" "$SRC_DIR/openssl-$OPENSSL_VERSION"

  download_tarball "$SRC_DIR/zlib-$ZLIB_VERSION.tar.gz" \
    "https://zlib.net/zlib-$ZLIB_VERSION.tar.gz" \
    "https://zlib.net/fossils/zlib-$ZLIB_VERSION.tar.gz"
  extract_tarball "$SRC_DIR/zlib-$ZLIB_VERSION.tar.gz" "$SRC_DIR/zlib-$ZLIB_VERSION"

  download_tarball "$SRC_DIR/cjson-$CJSON_VERSION.tar.gz" \
    "https://github.com/DaveGamble/cJSON/archive/refs/tags/v$CJSON_VERSION.tar.gz"
  extract_tarball "$SRC_DIR/cjson-$CJSON_VERSION.tar.gz" "$SRC_DIR/cJSON-$CJSON_VERSION"

  if is_enabled "$ENABLE_URIPARSER"; then
    download_archive "$SRC_DIR/uriparser-$URIPARSER_VERSION.tar.xz" \
      "https://github.com/uriparser/uriparser/releases/download/uriparser-$URIPARSER_VERSION/uriparser-$URIPARSER_VERSION.tar.xz"
    extract_archive "$SRC_DIR/uriparser-$URIPARSER_VERSION.tar.xz" "$SRC_DIR/uriparser-$URIPARSER_VERSION"
  fi

  if is_enabled "$ENABLE_OPENH264"; then
    download_tarball "$SRC_DIR/openh264-$OPENH264_VERSION.tar.gz" \
      "https://github.com/cisco/openh264/archive/refs/tags/v$OPENH264_VERSION.tar.gz"
    extract_tarball "$SRC_DIR/openh264-$OPENH264_VERSION.tar.gz" "$SRC_DIR/openh264-$OPENH264_VERSION"
  fi

  if is_enabled "$ENABLE_FFMPEG"; then
    download_archive "$SRC_DIR/ffmpeg-$FFMPEG_VERSION.tar.xz" \
      "https://ffmpeg.org/releases/ffmpeg-$FFMPEG_VERSION.tar.xz"
    extract_archive "$SRC_DIR/ffmpeg-$FFMPEG_VERSION.tar.xz" "$SRC_DIR/ffmpeg-$FFMPEG_VERSION"
  fi

  [[ -f "$FREE_RDP_SRC/CMakeLists.txt" ]] || {
    printf 'FreeRDP source is missing: %s\n' "$FREE_RDP_SRC" >&2
    exit 1
  }
}

cmake_common_args=()

prepare_cmake_args() {
  cmake_common_args=(
    "-G" "Ninja"
    "-DCMAKE_TOOLCHAIN_FILE=$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake"
    "-DCMAKE_C_COMPILER=$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang"
    "-DCMAKE_CXX_COMPILER=$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++"
    "-DOHOS_ARCH=$OHOS_ARCH"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_C_FLAGS=-Qunused-arguments"
    "-DCMAKE_CXX_FLAGS=-Qunused-arguments"
    "-DCMAKE_INSTALL_PREFIX=$PREFIX"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_PREFIX_PATH=$PREFIX"
    "-DCMAKE_FIND_ROOT_PATH=$PREFIX;$OHOS_NDK_HOME"
    "-DPKG_CONFIG_EXECUTABLE=$(command -v pkg-config)"
  )

  if [[ -n "$CCACHE_LAUNCHER" ]]; then
    cmake_common_args+=(
      "-DCMAKE_C_COMPILER_LAUNCHER=$CCACHE_LAUNCHER"
      "-DCMAKE_CXX_COMPILER_LAUNCHER=$CCACHE_LAUNCHER"
    )
  fi
}

prepare_freerdp_build_source() {
  FREERDP_BUILD_SRC="$BUILD_DIR/freerdp-src"
  safe_rm_rf "$FREERDP_BUILD_SRC"
  mkdir -p "$FREERDP_BUILD_SRC"
  if git -C "$FREE_RDP_SRC" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    if [[ -n "$(git -C "$FREE_RDP_SRC" status --porcelain)" ]]; then
      log "copy FreeRDP working tree with local changes"
      cp -a "$FREE_RDP_SRC/." "$FREERDP_BUILD_SRC/"
      safe_rm_rf "$FREERDP_BUILD_SRC/.git"
    else
      git -C "$FREE_RDP_SRC" archive --format=tar HEAD | tar -x -C "$FREERDP_BUILD_SRC"
    fi
  else
    cp -a "$FREE_RDP_SRC/." "$FREERDP_BUILD_SRC/"
  fi
}

build_openssl() {
  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libssl.so" && -f "$PREFIX/include/openssl/ssl.h" ]]; then
    log "OpenSSL already installed"
    return 0
  fi

  local src="$SRC_DIR/openssl-$OPENSSL_VERSION"
  local build="$BUILD_DIR/openssl-$OPENSSL_VERSION"
  safe_rm_rf "$build"
  mkdir -p "$build"
  cp -a "$src/." "$build/"

  log "build OpenSSL $OPENSSL_VERSION for $OHOS_ARCH"
  (
    cd "$build"
    unset CROSS_COMPILE
    export CC="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang"
    export CXX="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++"
    export AR="$OHOS_LLVM_HOME/bin/llvm-ar"
    export RANLIB="$OHOS_LLVM_HOME/bin/llvm-ranlib"
    export NM="$OHOS_LLVM_HOME/bin/llvm-nm"
    export STRIP="$OHOS_LLVM_HOME/bin/llvm-strip"

    perl Configure linux-aarch64 \
      --prefix="$PREFIX" \
      --openssldir="$PREFIX/ssl" \
      shared no-apps no-docs no-tests \
      -D__MUSL__ \
      2>&1 | tee "$LOG_DIR/openssl-configure.log"

    make -j"$JOBS" 2>&1 | tee "$LOG_DIR/openssl-build.log"
    make install_sw install_ssldirs 2>&1 | tee "$LOG_DIR/openssl-install.log"
  )
}

build_zlib() {
  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libz.so" && -f "$PREFIX/include/zlib.h" ]]; then
    log "zlib already installed"
    return 0
  fi

  local build="$BUILD_DIR/zlib-$ZLIB_VERSION"
  if [[ "$FORCE_REBUILD" == "1" ]]; then
    safe_rm_rf "$build"
  fi
  log "build zlib $ZLIB_VERSION for $OHOS_ARCH"
  cmake -S "$SRC_DIR/zlib-$ZLIB_VERSION" -B "$build" "${cmake_common_args[@]}" \
    -DBUILD_SHARED_LIBS=ON \
    -DZLIB_BUILD_EXAMPLES=OFF \
    2>&1 | tee "$LOG_DIR/zlib-configure.log"
  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/zlib-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/zlib-install.log"
}

build_cjson() {
  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libcjson.so" && -f "$PREFIX/include/cjson/cJSON.h" ]]; then
    log "cJSON already installed"
    return 0
  fi

  local build="$BUILD_DIR/cjson-$CJSON_VERSION"
  if [[ "$FORCE_REBUILD" == "1" ]]; then
    safe_rm_rf "$build"
  fi
  log "build cJSON $CJSON_VERSION for $OHOS_ARCH"
  cmake -S "$SRC_DIR/cJSON-$CJSON_VERSION" -B "$build" "${cmake_common_args[@]}" \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_CUSTOM_COMPILER_FLAGS=OFF \
    -DENABLE_CJSON_TEST=OFF \
    -DENABLE_CJSON_UTILS=OFF \
    -DENABLE_TARGET_EXPORT=ON \
    2>&1 | tee "$LOG_DIR/cjson-configure.log"
  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/cjson-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/cjson-install.log"
}

build_uriparser() {
  if ! is_enabled "$ENABLE_URIPARSER"; then
    log "uriparser disabled"
    return 0
  fi

  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/liburiparser.so" && -f "$PREFIX/include/uriparser/Uri.h" ]]; then
    log "uriparser already installed"
    return 0
  fi

  local build="$BUILD_DIR/uriparser-$URIPARSER_VERSION"
  if [[ "$FORCE_REBUILD" == "1" ]]; then
    safe_rm_rf "$build"
  fi
  log "build uriparser $URIPARSER_VERSION for $OHOS_ARCH"
  cmake -S "$SRC_DIR/uriparser-$URIPARSER_VERSION" -B "$build" "${cmake_common_args[@]}" \
    -DBUILD_SHARED_LIBS=ON \
    -DURIPARSER_BUILD_DOCS=OFF \
    -DURIPARSER_BUILD_TESTS=OFF \
    -DURIPARSER_BUILD_TOOLS=OFF \
    2>&1 | tee "$LOG_DIR/uriparser-configure.log"
  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/uriparser-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/uriparser-install.log"
}

build_openh264() {
  if ! is_enabled "$ENABLE_OPENH264"; then
    log "OpenH264 disabled"
    return 0
  fi

  ensure_openh264_cmake_config() {
    local cmake_dir="$PREFIX/lib/cmake/OpenH264"
    mkdir -p "$cmake_dir"
    {
      printf 'set(OpenH264_FOUND TRUE)\n'
      printf 'set(OPENH264_FOUND TRUE)\n'
      printf 'set(OPENH264_INCLUDE_DIR "%s/include")\n' "$PREFIX"
      printf 'set(OPENH264_INCLUDE_DIRS "%s/include")\n' "$PREFIX"
      printf 'set(OPENH264_LIBRARY "%s/lib/libopenh264.so")\n' "$PREFIX"
      printf 'set(OPENH264_LIBRARIES "%s/lib/libopenh264.so")\n' "$PREFIX"
    } >"$cmake_dir/OpenH264Config.cmake"
  }

  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libopenh264.so" && -f "$PREFIX/include/wels/codec_api.h" ]]; then
    log "OpenH264 already installed"
    ensure_openh264_cmake_config
    return 0
  fi

  local src="$SRC_DIR/openh264-$OPENH264_VERSION"
  local build="$BUILD_DIR/openh264-$OPENH264_VERSION"
  safe_rm_rf "$build"
  mkdir -p "$build"
  cp -a "$src/." "$build/"

  log "build OpenH264 $OPENH264_VERSION for $OHOS_ARCH"
  (
    cd "$build"
    make -j"$JOBS" \
      OS=linux \
      ARCH=arm64 \
      CC="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" \
      CXX="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++" \
      AR="$OHOS_LLVM_HOME/bin/llvm-ar" \
      PREFIX="$PREFIX" \
      2>&1 | tee "$LOG_DIR/openh264-build.log"
    make install-shared \
      OS=linux \
      ARCH=arm64 \
      CC="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" \
      CXX="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++" \
      AR="$OHOS_LLVM_HOME/bin/llvm-ar" \
      PREFIX="$PREFIX" \
      2>&1 | tee "$LOG_DIR/openh264-install.log"
  )
  ensure_openh264_cmake_config
}

build_ffmpeg() {
  if ! is_enabled "$ENABLE_FFMPEG"; then
    log "FFmpeg disabled"
    return 0
  fi

  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libavcodec.so" && -f "$PREFIX/lib/libavutil.so" ]]; then
    log "FFmpeg already installed"
    return 0
  fi

  local src="$SRC_DIR/ffmpeg-$FFMPEG_VERSION"
  local build="$BUILD_DIR/ffmpeg-$FFMPEG_VERSION"
  safe_rm_rf "$build"
  mkdir -p "$build"
  cp -a "$src/." "$build/"

  log "build FFmpeg $FFMPEG_VERSION for $OHOS_ARCH"
  (
    cd "$build"
    export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig"
    export PKG_CONFIG_SYSROOT_DIR=
    ./configure \
      --prefix="$PREFIX" \
      --enable-cross-compile \
      --target-os=linux \
      --arch=aarch64 \
      --cc="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" \
      --cxx="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++" \
      --ar="$OHOS_LLVM_HOME/bin/llvm-ar" \
      --ranlib="$OHOS_LLVM_HOME/bin/llvm-ranlib" \
      --nm="$OHOS_LLVM_HOME/bin/llvm-nm" \
      --strip="$OHOS_LLVM_HOME/bin/llvm-strip" \
      --pkg-config=pkg-config \
      --extra-cflags="-fPIC -D__MUSL__ -I$PREFIX/include" \
      --extra-ldflags="-L$PREFIX/lib" \
      --enable-pic \
      --enable-shared \
      --disable-static \
      --disable-programs \
      --disable-doc \
      --disable-debug \
      --disable-autodetect \
      --disable-iconv \
      --disable-bzlib \
      --disable-lzma \
      --disable-sdl2 \
      --disable-xlib \
      --enable-zlib \
      2>&1 | tee "$LOG_DIR/ffmpeg-configure.log"
    make -j"$JOBS" 2>&1 | tee "$LOG_DIR/ffmpeg-build.log"
    make install 2>&1 | tee "$LOG_DIR/ffmpeg-install.log"
  )
}

freerdp_feature_profile() {
  local freerdp_commit="unknown"
  freerdp_commit="$(git -C "$FREE_RDP_SRC" rev-parse HEAD 2>/dev/null || true)"
  {
    printf 'profile=%s\n' "$FREERDP_FEATURE_PROFILE"
    printf 'freerdp_commit=%s\n' "$freerdp_commit"
    printf 'uriparser=%s:%s\n' "$(cmake_bool "$ENABLE_URIPARSER")" "$URIPARSER_VERSION"
    printf 'openh264=%s:%s\n' "$(cmake_bool "$ENABLE_OPENH264")" "$OPENH264_VERSION"
    printf 'ffmpeg=%s:%s\n' "$(cmake_bool "$ENABLE_FFMPEG")" "$FFMPEG_VERSION"
    printf 'ohos_environment=ON\n'
    printf 'ohos_avcodec=%s\n' "$WITH_OHOS_AVCODEC"
    printf 'ohos_pasteboard=%s\n' "$WITH_OHOS_PASTEBOARD"
    printf 'ohos_print=%s\n' "$WITH_OHOS_PRINT"
    printf 'opensles=%s\n' "$WITH_OPENSLES"
    printf 'cups=%s\n' "$(cmake_bool "$ENABLE_CUPS")"
    printf 'smartcard=%s\n' "$(cmake_bool "$ENABLE_SMARTCARD")"
    printf 'pcsc=%s\n' "$(cmake_bool "$ENABLE_PCSC")"
    printf 'smartcard_pcsc=%s\n' "$(cmake_bool "$ENABLE_SMARTCARD_PCSC")"
    printf 'fuse=%s\n' "$(cmake_bool "$ENABLE_FUSE")"
  }
}

build_freerdp() {
  local build="$BUILD_DIR/freerdp"
  local feature_stamp="$PREFIX/.freerdp-feature-profile"
  local freerdp_dirty=0
  if git -C "$FREE_RDP_SRC" rev-parse --is-inside-work-tree >/dev/null 2>&1 &&
    [[ -n "$(git -C "$FREE_RDP_SRC" status --porcelain)" ]]; then
    freerdp_dirty=1
  fi

  if [[ "$FORCE_REBUILD" != "1" && "$freerdp_dirty" != "1" &&
        -f "$PREFIX/lib/libfreerdp3.so" && -f "$PREFIX/lib/libwinpr3.so" &&
        -f "$feature_stamp" && "$(freerdp_feature_profile)" == "$(cat "$feature_stamp")" ]]; then
    log "FreeRDP already installed"
    return 0
  fi

  safe_rm_rf "$build"
  prepare_freerdp_build_source

  log "build FreeRDP for $OHOS_ARCH"
  export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig"
  export PKG_CONFIG_SYSROOT_DIR=

  cmake -S "$FREERDP_BUILD_SRC" -B "$build" "${cmake_common_args[@]}" \
    -DUNIX=ON \
    -DOHOS=ON \
    -DCMAKE_MODULE_PATH="$FREERDP_BUILD_SRC/cmake" \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_TESTING_INTERNAL=OFF \
    -DBUILD_BENCHMARK=OFF \
    -DBUILD_FUZZERS=OFF \
    -DUSE_VERSION_FROM_GIT_TAG=OFF \
    -DUSE_GIT_FOR_REVISION=OFF \
    -DWITH_LIBRARY_VERSIONING=ON \
    -DWITH_LIBRARY_SOVERSIONING=OFF \
    -DWITH_BINARY_VERSIONING=OFF \
    -DWITH_MANPAGES=OFF \
    -DWITH_SAMPLE=OFF \
    -DWITH_SERVER=OFF \
    -DWITH_SERVER_INTERFACE=OFF \
    -DWITH_CHANNELS=ON \
    -DWITH_CLIENT_COMMON=ON \
    -DWITH_CLIENT=OFF \
    -DWITH_CLIENT_SDL=OFF \
    -DWITH_CLIENT_CHANNELS=ON \
    -DWITH_THIRD_PARTY=OFF \
    -DWITH_OPENSSL=ON \
    -DWITH_MBEDTLS=OFF \
    -DOPENSSL_ROOT_DIR="$PREFIX" \
    -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
    -DOPENSSL_SSL_LIBRARY="$PREFIX/lib/libssl.so" \
    -DOPENSSL_CRYPTO_LIBRARY="$PREFIX/lib/libcrypto.so" \
    -DWITH_JSON_DISABLED=OFF \
    -DWITH_CJSON_REQUIRED=ON \
    -DWITH_AAD=ON \
    -DWITH_FFMPEG="$(cmake_bool "$ENABLE_FFMPEG")" \
    -DWITH_DSP_FFMPEG="$(cmake_bool "$ENABLE_FFMPEG")" \
    -DWITH_VIDEO_FFMPEG="$(cmake_bool "$ENABLE_FFMPEG")" \
    -DWITH_SWSCALE="$(cmake_bool "$ENABLE_FFMPEG")" \
    -DWITH_CAIRO=OFF \
    -DWITH_JPEG=OFF \
    -DWITH_OPENH264="$(cmake_bool "$ENABLE_OPENH264")" \
    -DWITH_OHOS_AVCODEC="$WITH_OHOS_AVCODEC" \
    -DWITH_OHOS_PASTEBOARD="$WITH_OHOS_PASTEBOARD" \
    -DWITH_OHOS_PRINT="$WITH_OHOS_PRINT" \
    -DWITH_GFX_AV1=OFF \
    -DWITH_ALSA=OFF \
    -DWITH_PULSE=OFF \
    -DWITH_OSS=OFF \
    -DWITH_OHAUDIO="$WITH_OHAUDIO" \
    -DWITH_OPENSLES="$WITH_OPENSLES" \
    -DWITH_CUPS="$(cmake_bool "$ENABLE_CUPS")" \
    -DWITH_FUSE="$(cmake_bool "$ENABLE_FUSE")" \
    -DWITH_GSSAPI=OFF \
    -DWITH_KRB5=OFF \
    -DWITH_PKCS11=OFF \
    -DWITH_SMARTCARD=OFF \
    -DWITH_PCSC=OFF \
    -DWITH_PCSC_WINPR=OFF \
    -DWITH_SMARTCARD_EMULATE=OFF \
    -DWITH_SMARTCARD_INSPECT=OFF \
    -DWITH_SMARTCARD_PCSC=OFF \
    -DWITH_WINPR_TOOLS=OFF \
    -DWITH_URIPARSER="$(cmake_bool "$ENABLE_URIPARSER")" \
    -DWITH_UNICODE_BUILTIN=ON \
    -DWITH_X11=OFF \
    -DWITH_WAYLAND=OFF \
    -DCHANNEL_CLIPRDR=ON \
    -DCHANNEL_CLIPRDR_CLIENT=ON \
    -DCHANNEL_DRDYNVC=ON \
    -DCHANNEL_DRDYNVC_CLIENT=ON \
    -DCHANNEL_DISP=ON \
    -DCHANNEL_DISP_CLIENT=ON \
    -DCHANNEL_RDPGFX=ON \
    -DCHANNEL_RDPGFX_CLIENT=ON \
    -DCHANNEL_RDPSND=ON \
    -DCHANNEL_RDPSND_CLIENT=ON \
    -DCHANNEL_AUDIN=ON \
    -DCHANNEL_AUDIN_CLIENT=ON \
    -DCHANNEL_RDPDR=ON \
    -DCHANNEL_RDPDR_CLIENT=ON \
    -DCHANNEL_DRIVE=ON \
    -DCHANNEL_DRIVE_CLIENT=ON \
    -DCHANNEL_PRINTER=ON \
    -DCHANNEL_PRINTER_CLIENT=ON \
    -DCHANNEL_SMARTCARD=OFF \
    -DCHANNEL_SMARTCARD_CLIENT=OFF \
    -DCHANNEL_TSMF=OFF \
    -DCHANNEL_TSMF_CLIENT=OFF \
    -DCHANNEL_AINPUT=OFF \
    -DCHANNEL_ECHO=ON \
    -DCHANNEL_ECHO_CLIENT=ON \
    -DCHANNEL_ENCOMSP=OFF \
    -DCHANNEL_GEOMETRY=ON \
    -DCHANNEL_GEOMETRY_CLIENT=ON \
    -DCHANNEL_GFXREDIR=OFF \
    -DCHANNEL_LOCATION=ON \
    -DCHANNEL_LOCATION_CLIENT=ON \
    -DCHANNEL_PARALLEL=OFF \
    -DCHANNEL_RAIL=OFF \
    -DCHANNEL_RDP2TCP=OFF \
    -DCHANNEL_RDPEAR=OFF \
    -DCHANNEL_RDPECAM=ON \
    -DCHANNEL_RDPECAM_CLIENT=ON \
    -DCHANNEL_RDPEI=ON \
    -DCHANNEL_RDPEI_CLIENT=ON \
    -DCHANNEL_RDPEMSC=OFF \
    -DCHANNEL_RDPEWA=OFF \
    -DCHANNEL_REMDESK=OFF \
    -DCHANNEL_SERIAL=OFF \
    -DCHANNEL_SSHAGENT=OFF \
    -DCHANNEL_TELEMETRY=OFF \
    -DCHANNEL_URBDRC=OFF \
    -DCHANNEL_VIDEO=OFF \
    2>&1 | tee "$LOG_DIR/freerdp-configure.log"

  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/freerdp-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/freerdp-install.log"
  freerdp_feature_profile > "$feature_stamp"
}

copy_runtime_lib_pattern() {
  local pattern="$1"
  local required="${2:-0}"
  local copied=0
  shopt -s nullglob
  for lib in "$PREFIX/lib"/$pattern; do
    [[ -f "$lib" ]] || continue
    cp -L "$lib" "$RUNTIME_DIR/$(basename "$lib")"
    copied=1
  done
  shopt -u nullglob

  if [[ "$required" == "1" && "$copied" != "1" ]]; then
    printf 'Missing required runtime library pattern: %s\n' "$pattern" >&2
    exit 1
  fi
}

copy_runtime_soname_pattern() {
  local pattern="$1"
  local required="${2:-0}"
  local copied=0
  local readelf="$OHOS_LLVM_HOME/bin/llvm-readelf"
  shopt -s nullglob
  for lib in "$PREFIX/lib"/$pattern; do
    [[ -f "$lib" ]] || continue
    local soname
    soname="$("$readelf" -d "$lib" 2>/dev/null | sed -n 's/.*Library soname: \[\(.*\)\].*/\1/p' | head -n 1)"
    [[ -n "$soname" ]] || soname="$(basename "$lib")"
    if [[ -f "$RUNTIME_DIR/$soname" ]]; then
      copied=1
      continue
    fi
    cp -L "$lib" "$RUNTIME_DIR/$soname"
    copied=1
  done
  shopt -u nullglob

  if [[ "$required" == "1" && "$copied" != "1" ]]; then
    printf 'Missing required runtime library pattern: %s\n' "$pattern" >&2
    exit 1
  fi
}

copy_ohos_runtime_lib() {
  local name="$1"
  local required="${2:-0}"
  local copied=0
  local candidates=(
    "$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/$name"
    "$OHOS_LLVM_HOME/lib/$OHOS_TRIPLE/$name"
    "$OHOS_LLVM_HOME/lib/$OHOS_TRIPLE/c++/$name"
  )

  for lib in "${candidates[@]}"; do
    if [[ -f "$lib" ]]; then
      cp -L "$lib" "$RUNTIME_DIR/$name"
      copied=1
      break
    fi
  done

  if [[ "$required" == "1" && "$copied" != "1" ]]; then
    printf 'Missing required OHOS runtime library: %s\n' "$name" >&2
    exit 1
  fi
}

stage_runtime_libs() {
  log "stage runtime shared libraries"
  safe_rm_rf "$RUNTIME_DIR"
  mkdir -p "$RUNTIME_DIR/ossl-modules"

  copy_runtime_lib_pattern "libfreerdp3.so" 1
  copy_runtime_lib_pattern "libfreerdp-client3.so" 1
  copy_runtime_lib_pattern "libwinpr3.so" 1
  copy_runtime_lib_pattern "libssl.so.3" 1
  copy_runtime_lib_pattern "libcrypto.so.3" 1
  copy_runtime_lib_pattern "libcjson.so.1" 1
  copy_runtime_lib_pattern "libz.so.1" 1
  copy_runtime_soname_pattern "liburiparser.so*" "$(is_enabled "$ENABLE_URIPARSER" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libopenh264.so*" "$(is_enabled "$ENABLE_OPENH264" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libavcodec.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libavdevice.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libavfilter.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libavformat.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libavutil.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libswresample.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"
  copy_runtime_soname_pattern "libswscale.so*" "$(is_enabled "$ENABLE_FFMPEG" && printf 1 || printf 0)"

  if [[ "$WITH_OPENSLES" == "ON" ]]; then
    copy_ohos_runtime_lib "libOpenSLES.so" 1
  fi

  if is_enabled "$ENABLE_OPENH264"; then
    copy_ohos_runtime_lib "libc++_shared.so" 1
  fi

  if [[ -f "$PREFIX/lib/ossl-modules/legacy.so" ]]; then
    cp -L "$PREFIX/lib/ossl-modules/legacy.so" "$RUNTIME_DIR/ossl-modules/legacy.so"
  fi
}

write_manifest() {
  local manifest="$OUT_DIR/manifest.txt"
  local freerdp_commit="unknown"
  freerdp_commit="$(git -C "$FREE_RDP_SRC" rev-parse HEAD 2>/dev/null || true)"

  {
    printf 'build_time_utc=%s\n' "$(date -u +%FT%TZ)"
    printf 'ohos_arch=%s\n' "$OHOS_ARCH"
    printf 'ohos_ndk_home=%s\n' "$OHOS_NDK_HOME"
    printf 'freerdp_source=%s\n' "$FREE_RDP_SRC"
    printf 'freerdp_commit=%s\n' "$freerdp_commit"
    printf 'openssl_version=%s\n' "$OPENSSL_VERSION"
    printf 'zlib_version=%s\n' "$ZLIB_VERSION"
    printf 'cjson_version=%s\n' "$CJSON_VERSION"
    printf 'uriparser_version=%s\n' "$URIPARSER_VERSION"
    printf 'openh264_version=%s\n' "$OPENH264_VERSION"
    printf 'ffmpeg_version=%s\n' "$FFMPEG_VERSION"
    printf 'with_ohaudio=%s\n' "$WITH_OHAUDIO"
    printf 'with_ohos_environment=ON\n'
    printf 'with_ohos_avcodec=%s\n' "$WITH_OHOS_AVCODEC"
    printf 'with_ohos_pasteboard=%s\n' "$WITH_OHOS_PASTEBOARD"
    printf 'with_ohos_print=%s\n' "$WITH_OHOS_PRINT"
    printf 'with_opensles=%s\n' "$WITH_OPENSLES"
    printf 'with_cups=%s\n' "$(cmake_bool "$ENABLE_CUPS")"
    printf 'with_smartcard=%s\n' "$(cmake_bool "$ENABLE_SMARTCARD")"
    printf 'with_pcsc=%s\n' "$(cmake_bool "$ENABLE_PCSC")"
    printf 'with_smartcard_pcsc=%s\n' "$(cmake_bool "$ENABLE_SMARTCARD_PCSC")"
    printf 'with_fuse=%s\n' "$(cmake_bool "$ENABLE_FUSE")"
    printf 'enable_ccache=%s\n' "$ENABLE_CCACHE"
    printf 'ccache_launcher=%s\n' "${CCACHE_LAUNCHER:-none}"
    printf 'ccache_dir=%s\n' "${CCACHE_DIR:-}"
    printf '\n[libs]\n'
    find "$PREFIX/lib" -maxdepth 2 -type f \( -name '*.so' -o -name '*.so.*' \) -printf '%p\n' | sort
    printf '\n[runtime-libs]\n'
    find "$RUNTIME_DIR" -type f \( -name '*.so' -o -name '*.so.*' \) -printf '%p\n' | sort
  } > "$manifest"
}

verify_elf_outputs() {
  local report="$OUT_DIR/elf-report.txt"
  local readelf="$OHOS_LLVM_HOME/bin/llvm-readelf"
  : > "$report"

  log "verify OHOS ELF outputs"
  for lib in \
    "$PREFIX/lib"/libfreerdp*.so* \
    "$PREFIX/lib"/libwinpr*.so* \
    "$PREFIX/lib"/libssl*.so* \
    "$PREFIX/lib"/libcrypto*.so* \
    "$PREFIX/lib"/libcjson*.so* \
    "$PREFIX/lib"/libz*.so* \
    "$PREFIX/lib"/liburiparser*.so* \
    "$PREFIX/lib"/libopenh264*.so* \
    "$PREFIX/lib"/libav*.so* \
    "$PREFIX/lib"/libsw*.so* \
    "$RUNTIME_DIR"/*.so* \
    "$RUNTIME_DIR"/ossl-modules/*.so*; do
    [[ -e "$lib" ]] || continue
    {
      printf '\n## %s\n' "$lib"
      "$readelf" -h "$lib" | grep -E 'Class:|Machine:|Type:'
      "$readelf" -d "$lib" 2>/dev/null | grep -E 'NEEDED|SONAME|RUNPATH|RPATH' || true
    } | tee -a "$report"
  done
}

main() {
  load_ohos_env
  require_tool cmake
  require_tool ninja
  require_tool git
  require_tool perl
  require_tool make
  require_tool tar
  require_tool curl
  require_tool pkg-config

  configure_ccache
  prepare_sources
  prepare_cmake_args
  detect_optional_backends
  install_ohos_opensles_android_shim

  build_openssl
  build_zlib
  build_cjson
  build_uriparser
  build_openh264
  build_ffmpeg
  build_freerdp
  stage_runtime_libs
  write_manifest
  verify_elf_outputs

  log "done"
  printf 'Output: %s\n' "$OUT_DIR"
}

main "$@"
