#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
FREE_RDP_SRC="$REPO_ROOT/harmony/third_party/FreeRDP"
FREERDP_BUILD_SRC=""

OUT_DIR="${OUT_DIR:-$REPO_ROOT/harmony/out/ohos-arm64}"
PREFIX="$OUT_DIR/sysroot"
LOG_DIR="$OUT_DIR/logs"
PROBE_DIR="$OUT_DIR/probe"
RUNTIME_DIR="$OUT_DIR/runtime-libs"

OHOS_ARCH="${OHOS_ARCH:-arm64-v8a}"
WORK_DIR="${OHOS_BUILD_WORKDIR:-$HOME/.cache/demo-ohos-${OHOS_ARCH}}"
SRC_DIR="$WORK_DIR/src"
BUILD_DIR="$WORK_DIR/build"

OPENSSL_VERSION="${OPENSSL_VERSION:-3.3.2}"
ZLIB_VERSION="${ZLIB_VERSION:-1.3.1}"
CJSON_VERSION="${CJSON_VERSION:-1.7.18}"
JOBS="${JOBS:-$(nproc)}"
FORCE_REBUILD="${FORCE_REBUILD:-0}"

log() {
  printf '\n==> %s\n' "$*"
}

require_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'Missing required tool: %s\n' "$1" >&2
    exit 1
  fi
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

download_tarball() {
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

extract_tarball() {
  local archive="$1"
  local directory="$2"

  [[ -d "$directory" ]] && return 0
  log "extract $(basename "$archive")"
  tar -xzf "$archive" -C "$SRC_DIR"
}

prepare_sources() {
  mkdir -p "$SRC_DIR" "$BUILD_DIR" "$PREFIX" "$LOG_DIR" "$PROBE_DIR"

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
    "-DOHOS_ARCH=$OHOS_ARCH"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_INSTALL_PREFIX=$PREFIX"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_PREFIX_PATH=$PREFIX"
    "-DCMAKE_FIND_ROOT_PATH=$PREFIX;$OHOS_NDK_HOME"
  )
}

prepare_freerdp_build_source() {
  FREERDP_BUILD_SRC="$BUILD_DIR/freerdp-src"
  safe_rm_rf "$FREERDP_BUILD_SRC"
  mkdir -p "$FREERDP_BUILD_SRC"
  cp -a "$FREE_RDP_SRC/." "$FREERDP_BUILD_SRC/"

  local thread_file="$FREERDP_BUILD_SRC/winpr/libwinpr/thread/thread.c"
  if ! grep -q '__OHOS__' "$thread_file"; then
    perl -0pi -e 's/#ifndef ANDROID\s+pthread_cancel\(thread->thread\);\s+#else/#if !defined(ANDROID) \&\& !defined(__OHOS__)\n\tpthread_cancel(thread->thread);\n#else/s' "$thread_file"
    grep -q '__OHOS__' "$thread_file" || {
      printf 'Failed to apply OHOS pthread_cancel compatibility patch to %s\n' "$thread_file" >&2
      exit 1
    }
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
  log "build cJSON $CJSON_VERSION for $OHOS_ARCH"
  cmake -S "$SRC_DIR/cJSON-$CJSON_VERSION" -B "$build" "${cmake_common_args[@]}" \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_CJSON_TEST=OFF \
    -DENABLE_CJSON_UTILS=OFF \
    -DENABLE_TARGET_EXPORT=ON \
    2>&1 | tee "$LOG_DIR/cjson-configure.log"
  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/cjson-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/cjson-install.log"
}

build_freerdp() {
  local build="$BUILD_DIR/freerdp"

  if [[ "$FORCE_REBUILD" != "1" && -f "$PREFIX/lib/libfreerdp3.so" && -f "$PREFIX/lib/libwinpr3.so" ]]; then
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
    -DWITH_CHANNELS=OFF \
    -DWITH_CLIENT_COMMON=ON \
    -DWITH_CLIENT=OFF \
    -DWITH_CLIENT_SDL=OFF \
    -DWITH_CLIENT_CHANNELS=OFF \
    -DWITH_THIRD_PARTY=OFF \
    -DWITH_OPENSSL=ON \
    -DWITH_MBEDTLS=OFF \
    -DOPENSSL_ROOT_DIR="$PREFIX" \
    -DOPENSSL_INCLUDE_DIR="$PREFIX/include" \
    -DOPENSSL_SSL_LIBRARY="$PREFIX/lib/libssl.so" \
    -DOPENSSL_CRYPTO_LIBRARY="$PREFIX/lib/libcrypto.so" \
    -DZLIB_ROOT="$PREFIX" \
    -DZLIB_INCLUDE_DIR="$PREFIX/include" \
    -DZLIB_LIBRARY="$PREFIX/lib/libz.so" \
    -DWITH_JSON_DISABLED=OFF \
    -DWITH_CJSON_REQUIRED=ON \
    -DWITH_AAD=OFF \
    -DWITH_FFMPEG=OFF \
    -DWITH_DSP_FFMPEG=OFF \
    -DWITH_VIDEO_FFMPEG=OFF \
    -DWITH_SWSCALE=OFF \
    -DWITH_CAIRO=OFF \
    -DWITH_JPEG=OFF \
    -DWITH_OPENH264=OFF \
    -DWITH_GFX_AV1=OFF \
    -DWITH_ALSA=OFF \
    -DWITH_PULSE=OFF \
    -DWITH_OSS=OFF \
    -DWITH_CUPS=OFF \
    -DWITH_FUSE=OFF \
    -DWITH_GSSAPI=OFF \
    -DWITH_KRB5=OFF \
    -DWITH_PKCS11=OFF \
    -DWITH_PCSC=OFF \
    -DWITH_PCSC_WINPR=OFF \
    -DWITH_SMARTCARD_EMULATE=OFF \
    -DWITH_SMARTCARD_INSPECT=OFF \
    -DWITH_SMARTCARD_PCSC=OFF \
    -DWITH_WINPR_TOOLS=OFF \
    -DWITH_URIPARSER=OFF \
    -DWITH_UNICODE_BUILTIN=ON \
    -DWITH_X11=OFF \
    -DWITH_WAYLAND=OFF \
    2>&1 | tee "$LOG_DIR/freerdp-configure.log"

  cmake --build "$build" --parallel "$JOBS" 2>&1 | tee "$LOG_DIR/freerdp-build.log"
  cmake --install "$build" 2>&1 | tee "$LOG_DIR/freerdp-install.log"
}

stage_runtime_libs() {
  log "stage runtime shared libraries"
  safe_rm_rf "$RUNTIME_DIR"
  mkdir -p "$RUNTIME_DIR/ossl-modules"

  cp -L "$PREFIX/lib/libfreerdp3.so" "$RUNTIME_DIR/libfreerdp3.so"
  cp -L "$PREFIX/lib/libfreerdp-client3.so" "$RUNTIME_DIR/libfreerdp-client3.so"
  cp -L "$PREFIX/lib/libwinpr3.so" "$RUNTIME_DIR/libwinpr3.so"
  cp -L "$PREFIX/lib/libssl.so.3" "$RUNTIME_DIR/libssl.so.3"
  cp -L "$PREFIX/lib/libcrypto.so.3" "$RUNTIME_DIR/libcrypto.so.3"
  cp -L "$PREFIX/lib/libcjson.so.1" "$RUNTIME_DIR/libcjson.so.1"
  cp -L "$PREFIX/lib/libz.so.1" "$RUNTIME_DIR/libz.so.1"

  if [[ -f "$PREFIX/lib/ossl-modules/legacy.so" ]]; then
    cp -L "$PREFIX/lib/ossl-modules/legacy.so" "$RUNTIME_DIR/ossl-modules/legacy.so"
  fi
}

build_probe() {
  local source="$BUILD_DIR/freerdp_ohos_probe.c"
  local output="$PROBE_DIR/libfreerdp_ohos_probe.so"
  mkdir -p "$PROBE_DIR"

  cat > "$source" <<'EOF'
#include <cjson/cJSON.h>
#include <freerdp/freerdp.h>
#include <openssl/crypto.h>
#include <winpr/winpr.h>

#include <stdio.h>

__attribute__((visibility("default"))) const char* freerdp_ohos_probe(void)
{
    static char fallback[512];
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        return "{\"ok\":false,\"error\":\"cJSON_CreateObject failed\"}";
    }

    int freerdp_major = 0;
    int freerdp_minor = 0;
    int freerdp_revision = 0;
    freerdp_get_version(&freerdp_major, &freerdp_minor, &freerdp_revision);

    int winpr_major = 0;
    int winpr_minor = 0;
    int winpr_revision = 0;
    winpr_get_version(&winpr_major, &winpr_minor, &winpr_revision);

    cJSON_AddBoolToObject(root, "ok", 1);
    cJSON_AddStringToObject(root, "module", "freerdp_ohos_probe");
    cJSON_AddStringToObject(root, "freerdpVersion", freerdp_get_version_string());
    cJSON_AddStringToObject(root, "winprVersion", winpr_get_version_string());
    cJSON_AddStringToObject(root, "opensslVersion", OpenSSL_version(OPENSSL_VERSION));
    cJSON_AddNumberToObject(root, "freerdpMajor", freerdp_major);
    cJSON_AddNumberToObject(root, "freerdpMinor", freerdp_minor);
    cJSON_AddNumberToObject(root, "freerdpRevision", freerdp_revision);
    cJSON_AddNumberToObject(root, "winprMajor", winpr_major);
    cJSON_AddNumberToObject(root, "winprMinor", winpr_minor);
    cJSON_AddNumberToObject(root, "winprRevision", winpr_revision);

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        return "{\"ok\":false,\"error\":\"cJSON_PrintUnformatted failed\"}";
    }

    snprintf(fallback, sizeof(fallback), "%s", json);
    cJSON_free(json);
    return fallback;
}
EOF

  log "build OHOS probe shared library"
  "$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" \
    -fPIC -shared "$source" -o "$output" \
    -I"$PREFIX/include" \
    -I"$PREFIX/include/freerdp3" \
    -I"$PREFIX/include/winpr3" \
    -L"$PREFIX/lib" \
    -Wl,-rpath,'$ORIGIN/../sysroot/lib' \
    -lfreerdp3 -lwinpr3 -lssl -lcrypto -lcjson -lz \
    2>&1 | tee "$LOG_DIR/probe-build.log"
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
    printf '\n[libs]\n'
    find "$PREFIX/lib" "$PROBE_DIR" -maxdepth 1 -type f \( -name '*.so' -o -name '*.so.*' \) -printf '%p\n' | sort
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
    "$PROBE_DIR"/libfreerdp_ohos_probe.so \
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
  require_tool perl
  require_tool make
  require_tool curl
  require_tool tar
  require_tool git

  prepare_sources
  prepare_cmake_args

  build_openssl
  build_zlib
  build_cjson
  build_freerdp
  stage_runtime_libs
  build_probe
  write_manifest
  verify_elf_outputs

  log "done"
  printf 'Output: %s\n' "$OUT_DIR"
}

main "$@"
