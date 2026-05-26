#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
FREE_RDP_SRC="$REPO_ROOT/harmony/third_party/FreeRDP"
BASE_OUT_DIR="${BASE_OUT_DIR:-$REPO_ROOT/harmony/out/ohos-arm64}"
PREFIX="${PREFIX:-$BASE_OUT_DIR/sysroot}"
MATRIX_DIR="${MATRIX_DIR:-$REPO_ROOT/harmony/out/freerdp-feature-matrix}"
OHOS_ARCH="${OHOS_ARCH:-arm64-v8a}"
OHOS_TRIPLE="${OHOS_TRIPLE:-aarch64-linux-ohos}"
MATRIX_BUILD="${MATRIX_BUILD:-1}"
JOBS="${JOBS:-$(nproc)}"

log() {
  printf '\n==> %s\n' "$*"
}

load_ohos_env() {
  if [[ -z "${OHOS_NDK_HOME:-}" && -f /etc/profile.d/ohos-sdk.sh ]]; then
    # shellcheck disable=SC1091
    . /etc/profile.d/ohos-sdk.sh
  fi

  : "${OHOS_NDK_HOME:?Set OHOS_NDK_HOME to the Linux OHOS native SDK path}"
  : "${OHOS_LLVM_HOME:=$OHOS_NDK_HOME/llvm}"

  export PATH="$OHOS_NDK_HOME/build-tools/cmake/bin:$OHOS_LLVM_HOME/bin:/usr/local/bin:/usr/bin:/bin:$PATH"
}

safe_rm_rf() {
  local target="$1"
  [[ -e "$target" ]] || return 0

  local resolved matrix_root
  resolved="$(realpath -m "$target")"
  matrix_root="$(realpath -m "$MATRIX_DIR")"

  case "$resolved" in
    "$matrix_root"/*|"$matrix_root")
      rm -rf "$resolved"
      ;;
    *)
      printf 'Refusing to remove path outside matrix dir: %s\n' "$resolved" >&2
      exit 1
      ;;
  esac
}

require_base_runtime() {
  local missing=0
  for path in \
    "$PREFIX/lib/libfreerdp3.so" \
    "$PREFIX/lib/libwinpr3.so" \
    "$PREFIX/lib/libssl.so" \
    "$PREFIX/lib/libcrypto.so" \
    "$PREFIX/lib/libz.so" \
    "$PREFIX/include/freerdp3/freerdp/freerdp.h"; do
    if [[ ! -e "$path" ]]; then
      printf 'Missing base FreeRDP sysroot artifact: %s\n' "$path" >&2
      missing=1
    fi
  done

  if [[ "$missing" == "1" ]]; then
    printf 'Run harmony/scripts/wsl/build-freerdp-ohos.sh first.\n' >&2
    exit 1
  fi
}

cmake_common_args=()
base_options=()

prepare_cmake_args() {
  cmake_common_args=(
    "-G" "Ninja"
    "-DCMAKE_TOOLCHAIN_FILE=$OHOS_NDK_HOME/build/cmake/ohos.toolchain.cmake"
    "-DOHOS_ARCH=$OHOS_ARCH"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_C_FLAGS=-Qunused-arguments"
    "-DCMAKE_CXX_FLAGS=-Qunused-arguments"
    "-DCMAKE_INSTALL_PREFIX=$MATRIX_DIR/install"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_PREFIX_PATH=$PREFIX"
    "-DCMAKE_FIND_ROOT_PATH=$PREFIX;$OHOS_NDK_HOME"
    "-DPKG_CONFIG_EXECUTABLE=$(command -v pkg-config)"
  )

  base_options=(
    -DUNIX=ON
    -DBUILD_SHARED_LIBS=ON
    -DBUILD_TESTING=OFF
    -DBUILD_TESTING_INTERNAL=OFF
    -DBUILD_BENCHMARK=OFF
    -DBUILD_FUZZERS=OFF
    -DUSE_VERSION_FROM_GIT_TAG=OFF
    -DUSE_GIT_FOR_REVISION=OFF
    -DWITH_LIBRARY_VERSIONING=ON
    -DWITH_LIBRARY_SOVERSIONING=OFF
    -DWITH_BINARY_VERSIONING=OFF
    -DWITH_MANPAGES=OFF
    -DWITH_SAMPLE=OFF
    -DWITH_SERVER=OFF
    -DWITH_SERVER_INTERFACE=OFF
    -DWITH_CHANNELS=ON
    -DWITH_CLIENT_COMMON=ON
    -DWITH_CLIENT=OFF
    -DWITH_CLIENT_SDL=OFF
    -DWITH_CLIENT_CHANNELS=ON
    -DWITH_THIRD_PARTY=OFF
    -DWITH_OPENSSL=ON
    -DWITH_MBEDTLS=OFF
    "-DOPENSSL_ROOT_DIR=$PREFIX"
    "-DOPENSSL_INCLUDE_DIR=$PREFIX/include"
    "-DOPENSSL_SSL_LIBRARY=$PREFIX/lib/libssl.so"
    "-DOPENSSL_CRYPTO_LIBRARY=$PREFIX/lib/libcrypto.so"
    "-DZLIB_ROOT=$PREFIX"
    "-DZLIB_INCLUDE_DIR=$PREFIX/include"
    "-DZLIB_LIBRARY=$PREFIX/lib/libz.so"
    -DWITH_JSON_DISABLED=OFF
    -DWITH_CJSON_REQUIRED=ON
    -DWITH_AAD=ON
    -DWITH_FFMPEG=ON
    -DWITH_DSP_FFMPEG=ON
    -DWITH_VIDEO_FFMPEG=ON
    -DWITH_SWSCALE=ON
    -DWITH_CAIRO=OFF
    -DWITH_JPEG=OFF
    -DWITH_OPENH264=ON
    -DWITH_OHOS_PASTEBOARD=ON
    -DWITH_OHOS_PRINT=ON
    -DWITH_GFX_AV1=OFF
    -DWITH_ALSA=OFF
    -DWITH_PULSE=OFF
    -DWITH_OSS=OFF
    -DWITH_OHAUDIO=ON
    -DWITH_OPENSLES=OFF
    "-DOpenSLES_INCLUDE_DIR=$OHOS_NDK_HOME/sysroot/usr/include"
    "-DOpenSLES_LIBRARY=$OHOS_NDK_HOME/sysroot/usr/lib/$OHOS_TRIPLE/libOpenSLES.so"
    -DWITH_GSSAPI=OFF
    -DWITH_KRB5=OFF
    -DWITH_PKCS11=OFF
    -DWITH_SMARTCARD=OFF
    -DWITH_SMARTCARD_EMULATE=OFF
    -DWITH_SMARTCARD_INSPECT=OFF
    -DWITH_SMARTCARD_PCSC=OFF
    -DWITH_WINPR_TOOLS=OFF
    -DWITH_URIPARSER=ON
    -DWITH_UNICODE_BUILTIN=ON
    -DWITH_X11=OFF
    -DWITH_WAYLAND=OFF
    -DCHANNEL_CLIPRDR=ON
    -DCHANNEL_CLIPRDR_CLIENT=ON
    -DCHANNEL_DRDYNVC=ON
    -DCHANNEL_DRDYNVC_CLIENT=ON
    -DCHANNEL_DISP=ON
    -DCHANNEL_DISP_CLIENT=ON
    -DCHANNEL_RDPGFX=ON
    -DCHANNEL_RDPGFX_CLIENT=ON
    -DCHANNEL_RDPSND=ON
    -DCHANNEL_RDPSND_CLIENT=ON
    -DCHANNEL_AUDIN=ON
    -DCHANNEL_AUDIN_CLIENT=ON
    -DCHANNEL_RDPDR=ON
    -DCHANNEL_RDPDR_CLIENT=ON
    -DCHANNEL_DRIVE=ON
    -DCHANNEL_DRIVE_CLIENT=ON
    -DCHANNEL_PRINTER=ON
    -DCHANNEL_PRINTER_CLIENT=ON
    -DCHANNEL_SMARTCARD=OFF
    -DCHANNEL_SMARTCARD_CLIENT=OFF
    -DCHANNEL_TSMF=OFF
    -DCHANNEL_TSMF_CLIENT=OFF
    -DCHANNEL_AINPUT=OFF
    -DCHANNEL_ECHO=OFF
    -DCHANNEL_ENCOMSP=OFF
    -DCHANNEL_GEOMETRY=OFF
    -DCHANNEL_GFXREDIR=OFF
    -DCHANNEL_LOCATION=ON
    -DCHANNEL_LOCATION_CLIENT=ON
    -DCHANNEL_PARALLEL=OFF
    -DCHANNEL_RAIL=OFF
    -DCHANNEL_RDP2TCP=OFF
    -DCHANNEL_RDPEAR=OFF
    -DCHANNEL_RDPECAM=OFF
    -DCHANNEL_RDPEI=OFF
    -DCHANNEL_RDPEMSC=OFF
    -DCHANNEL_RDPEWA=OFF
    -DCHANNEL_REMDESK=OFF
    -DCHANNEL_SERIAL=OFF
    -DCHANNEL_SSHAGENT=OFF
    -DCHANNEL_TELEMETRY=OFF
    -DCHANNEL_URBDRC=OFF
    -DCHANNEL_VIDEO=OFF
  )
}

extract_reason() {
  local log_file="$1"
  grep -Ei 'Could NOT find|not found|No package|REQUIRED|fatal error|error:' "$log_file" |
    tail -n 8 |
    sed 's/|/\\|/g; s/^[[:space:]]*//; s/[[:space:]]*$//' |
    paste -sd '; ' -
}

run_case() {
  local name="$1"
  local description="$2"
  shift 2

  local case_dir="$MATRIX_DIR/$name"
  local build_dir="$case_dir/build"
  local configure_log="$case_dir/configure.log"
  local build_log="$case_dir/build.log"
  local status="configure-failed"
  local reason=""

  safe_rm_rf "$case_dir"
  mkdir -p "$build_dir"

  log "matrix case: $name"
  set +e
  PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig" \
  PKG_CONFIG_SYSROOT_DIR= \
    cmake -S "$FREE_RDP_SRC" -B "$build_dir" "${cmake_common_args[@]}" "${base_options[@]}" "$@" \
    >"$configure_log" 2>&1
  local configure_rc=$?
  set -e

  if [[ "$configure_rc" == "0" ]]; then
    status="configure-ok"
    if [[ "$MATRIX_BUILD" == "1" ]]; then
      set +e
      cmake --build "$build_dir" --parallel "$JOBS" >"$build_log" 2>&1
      local build_rc=$?
      set -e
      if [[ "$build_rc" == "0" ]]; then
        status="build-ok"
      else
        status="build-failed"
        reason="$(extract_reason "$build_log")"
      fi
    fi
  else
    reason="$(extract_reason "$configure_log")"
  fi

  if [[ -z "$reason" ]]; then
    reason="-"
  fi

  printf '| `%s` | %s | %s | %s |\n' "$name" "$status" "$description" "$reason" >> "$MATRIX_DIR/report.md"
  printf '%s: %s\n' "$name" "$status"
}

main() {
  load_ohos_env
  require_base_runtime
  prepare_cmake_args

  safe_rm_rf "$MATRIX_DIR"
  mkdir -p "$MATRIX_DIR"
  {
    printf '# FreeRDP OHOS Feature Matrix\n\n'
    printf '%s\n' "- Base sysroot: \`$PREFIX\`"
    printf '%s\n' "- FreeRDP source: \`$FREE_RDP_SRC\`"
    printf '%s\n\n' "- Matrix build: \`$MATRIX_BUILD\`"
    printf '| Case | Status | Scope | Reason |\n'
    printf '| --- | --- | --- | --- |\n'
    printf '| `enhanced-runtime` | build-ok | Current committed profile: cliprdr, disp, location, rdpgfx, rdpsnd with OHAudio, audin, rdpdr/drive, printer channel with OHOS native backend, FFmpeg, OpenH264; smartcard source/channel/PCSC and TSMF excluded from the delivery build | Proven by `harmony/scripts/wsl/build-freerdp-ohos.sh` |\n'
    printf '| `smartcard-pcsc` | skipped | Smartcard source, channel, and WinPR PCSC backend are excluded from the delivery build | Do not compile for prelaunch package |\n'
    printf '| `tsmf` | skipped | TSMF channel is excluded from the delivery build | Do not compile for prelaunch package |\n'
  } > "$MATRIX_DIR/report.md"

  run_case "cups" "Enable CUPS printer backend in addition to printer channel" \
    -DWITH_CUPS=ON -DWITH_FUSE=OFF -DWITH_PCSC=OFF -DWITH_PCSC_WINPR=OFF

  run_case "fuse" "Enable FUSE clipboard file-copy backend" \
    -DWITH_CUPS=OFF -DWITH_FUSE=ON -DWITH_PCSC=OFF -DWITH_PCSC_WINPR=OFF

  log "matrix report"
  cat "$MATRIX_DIR/report.md"
}

main "$@"
