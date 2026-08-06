#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
XRDP_SRC="${XRDP_SRC:-$REPO_ROOT/harmony/third_party/xrdp}"

OUT_DIR="${OUT_DIR:-$REPO_ROOT/harmony/out/xrdp-ohos-arm64}"
PREFIX="${XRDP_PREFIX:-$OUT_DIR/sysroot}"
CONFIG_DIR="${XRDP_CONFIG_DIR:-$OUT_DIR/config}"
VAR_DIR="${XRDP_VAR_DIR:-$OUT_DIR/var}"
RUN_DIR="${XRDP_RUN_DIR:-$OUT_DIR/run}"
BUILD_DIR="${XRDP_BUILD_DIR:-$OUT_DIR/build}"
LOG_DIR="$OUT_DIR/logs"

XRDP_DEPS_PREFIX="${XRDP_DEPS_PREFIX:-$REPO_ROOT/harmony/out/ohos-arm64/sysroot}"
OHOS_ARCH="${OHOS_ARCH:-arm64-v8a}"
OHOS_TRIPLE="${OHOS_TRIPLE:-aarch64-linux-ohos}"
OHOS_AUTOTOOLS_HOST="${OHOS_AUTOTOOLS_HOST:-aarch64-unknown-linux-musl}"
JOBS="${JOBS:-$(nproc)}"
FORCE_REBUILD="${FORCE_REBUILD:-0}"
ENABLE_CCACHE="${ENABLE_CCACHE:-auto}"
ENABLE_OPENH264="${ENABLE_XRDP_OPENH264:-1}"
CCACHE_PROGRAM="${CCACHE_PROGRAM:-ccache}"
CCACHE_LAUNCHER=""

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

configure_ccache() {
  CCACHE_LAUNCHER=""

  if ! is_enabled "$ENABLE_CCACHE" && ! is_auto "$ENABLE_CCACHE"; then
    log "ccache disabled"
    return 0
  fi

  if command -v "$CCACHE_PROGRAM" >/dev/null 2>&1; then
    CCACHE_LAUNCHER="$(command -v "$CCACHE_PROGRAM")"
    export CCACHE_DIR="${CCACHE_DIR:-$OUT_DIR/ccache}"
    export CCACHE_BASEDIR="${CCACHE_BASEDIR:-$REPO_ROOT}"
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

  local resolved repo_out
  resolved="$(realpath -m "$target")"
  repo_out="$(realpath -m "$REPO_ROOT/harmony/out")"

  case "$resolved" in
    "$repo_out"/*)
      rm -rf "$resolved"
      ;;
    *)
      printf 'Refusing to remove path outside harmony/out: %s\n' "$resolved" >&2
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

  [[ -x "$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang" ]] || {
    printf 'OHOS arm64 clang wrapper was not found under %s\n' "$OHOS_LLVM_HOME" >&2
    exit 1
  }
}

ensure_inputs() {
  [[ -d "$XRDP_SRC" ]] || {
    printf 'xrdp source directory was not found: %s\n' "$XRDP_SRC" >&2
    exit 1
  }

  [[ -f "$XRDP_DEPS_PREFIX/lib/pkgconfig/openssl.pc" ]] || {
    printf 'OpenSSL pkg-config file was not found: %s\n' "$XRDP_DEPS_PREFIX/lib/pkgconfig/openssl.pc" >&2
    printf 'Build the OHOS dependency sysroot first, for example with harmony/scripts/wsl/build-freerdp-ohos.sh.\n' >&2
    exit 1
  }
}

needs_bootstrap() {
  [[ -x "$XRDP_SRC/configure" ]] || return 0
  [[ "$XRDP_SRC/configure.ac" -nt "$XRDP_SRC/configure" ]] && return 0

  local am inf
  while IFS= read -r am; do
    inf="${am%.am}.in"
    if [[ ! -f "$inf" || "$am" -nt "$inf" ]]; then
      return 0
    fi
  done < <(find "$XRDP_SRC" -name Makefile.am -print)

  return 1
}

bootstrap_xrdp() {
  if needs_bootstrap; then
    require_tool autoreconf
    require_tool automake
    require_tool autoconf
    require_tool libtoolize
    log "bootstrap xrdp autotools files"
    (cd "$XRDP_SRC" && ./bootstrap) 2>&1 | tee "$LOG_DIR/xrdp-bootstrap.log"
  else
    log "xrdp autotools files are current"
  fi
}

configure_xrdp() {
  local cc_cmd cxx_cmd
  cc_cmd="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang"
  cxx_cmd="$OHOS_LLVM_HOME/bin/aarch64-unknown-linux-ohos-clang++"

  if [[ -n "$CCACHE_LAUNCHER" ]]; then
    cc_cmd="$CCACHE_LAUNCHER $cc_cmd"
    cxx_cmd="$CCACHE_LAUNCHER $cxx_cmd"
  fi

  export PKG_CONFIG_PATH="$XRDP_DEPS_PREFIX/lib/pkgconfig:$XRDP_DEPS_PREFIX/share/pkgconfig:${PKG_CONFIG_PATH:-}"
  export CPPFLAGS="-I$OHOS_NDK_HOME/sysroot/usr/include -I$XRDP_DEPS_PREFIX/include ${CPPFLAGS:-}"
  export CFLAGS="-D__MUSL__ -O2 ${CFLAGS:-}"
  export LDFLAGS="-L$XRDP_DEPS_PREFIX/lib -Wl,-rpath,'\$\$ORIGIN' -Wl,-rpath,'\$\$ORIGIN/../lib' -Wl,-rpath,'\$\$ORIGIN/..' ${LDFLAGS:-}"
  export CC="$cc_cmd"
  export CXX="$cxx_cmd"
  export AR="$OHOS_LLVM_HOME/bin/llvm-ar"
  export RANLIB="$OHOS_LLVM_HOME/bin/llvm-ranlib"
  export NM="$OHOS_LLVM_HOME/bin/llvm-nm"
  export STRIP="$OHOS_LLVM_HOME/bin/llvm-strip"

  local openh264_flag="--disable-openh264"
  if is_enabled "$ENABLE_OPENH264"; then
    if ! pkg-config --exists 'openh264 >= 2.0.0'; then
      printf 'OpenH264 was requested but openh264 >= 2.0.0 was not found under XRDP_DEPS_PREFIX: %s\n' "$XRDP_DEPS_PREFIX" >&2
      printf 'Build the OHOS dependency sysroot first, or set ENABLE_XRDP_OPENH264=0 to disable H.264.\n' >&2
      exit 1
    fi
    openh264_flag="--enable-openh264"
    log "xrdp OpenH264 enabled: $(pkg-config --modversion openh264)"
  else
    log "xrdp OpenH264 disabled"
  fi

  log "configure xrdp for OHOS"
  (
    cd "$BUILD_DIR"
    "$XRDP_SRC/configure" \
      --host="$OHOS_AUTOTOOLS_HOST" \
      --prefix="$PREFIX" \
      --exec-prefix="$PREFIX" \
      --bindir="$PREFIX/bin" \
      --sbindir="$PREFIX/sbin" \
      --libdir="$PREFIX/lib" \
      --includedir="$PREFIX/include" \
      --sysconfdir="$CONFIG_DIR" \
      --localstatedir="$VAR_DIR" \
      --runstatedir="$RUN_DIR" \
      --with-socketdir="$RUN_DIR/xrdp" \
      --with-systemdsystemunitdir=no \
      --enable-ohos \
      --enable-strict-locations \
      --disable-pam \
      --disable-pam-config \
      --disable-vsock \
      --disable-neutrinordp \
      --disable-jpeg \
      --disable-tjpeg \
      --disable-fuse \
      --disable-xrdpvr \
      --disable-fdkaac \
      --disable-opus \
      --disable-mp3lame \
      --disable-ibus \
      --disable-pixman \
      --disable-x264 \
      "$openh264_flag" \
      --disable-nvenc \
      --disable-accel \
      --disable-rdpsndaudin \
      --disable-utmp \
      --disable-smartcard \
      --enable-rfxcodec \
      --with-imlib2=no \
      --with-freetype2=no
  ) 2>&1 | tee "$LOG_DIR/xrdp-configure.log"
}

build_xrdp() {
  log "build xrdp OHOS minimal server"
  make -C "$BUILD_DIR" -j"$JOBS" 2>&1 | tee "$LOG_DIR/xrdp-build.log"
}

install_xrdp() {
  log "install xrdp OHOS minimal server"
  make -C "$BUILD_DIR" install 2>&1 | tee "$LOG_DIR/xrdp-install.log"
}

verify_outputs() {
  local server="$PREFIX/sbin/xrdp"
  local embedded_server="$PREFIX/lib/libxrdpserver.so"
  local backend="$PREFIX/lib/xrdp/libxrdpohos.so"
  local config="$CONFIG_DIR/xrdp/xrdp.ini"
  local nm_tool="$OHOS_LLVM_HOME/bin/llvm-nm"
  local backend_symbols="$LOG_DIR/xrdp-ohos-backend-nm.txt"
  local embedded_symbols="$LOG_DIR/xrdp-embedded-server-nm.txt"

  [[ -f "$server" ]] || {
    printf 'Missing xrdp server output: %s\n' "$server" >&2
    exit 1
  }
  [[ -f "$backend" ]] || {
    printf 'Missing OHOS backend output: %s\n' "$backend" >&2
    exit 1
  }
  [[ -f "$embedded_server" ]] || {
    printf 'Missing embedded xrdp server library: %s\n' "$embedded_server" >&2
    exit 1
  }
  [[ -f "$config" ]] || {
    printf 'Missing OHOS xrdp config output: %s\n' "$config" >&2
    exit 1
  }

  "$nm_tool" -D "$backend" > "$backend_symbols"
  "$nm_tool" -D "$embedded_server" > "$embedded_symbols"

  grep -q ' T mod_init' "$backend_symbols" || {
    printf 'OHOS backend does not export mod_init: %s\n' "$backend" >&2
    exit 1
  }
  grep -q ' T mod_exit' "$backend_symbols" || {
    printf 'OHOS backend does not export mod_exit: %s\n' "$backend" >&2
    exit 1
  }
  grep -q ' T xrdp_ohos_backend_set_rdpecam_callback' "$backend_symbols" || {
    printf 'OHOS backend does not export rdpecam callback ABI: %s\n' "$backend" >&2
    exit 1
  }
  grep -q ' T xrdp_ohos_server_main' "$embedded_symbols" || {
    printf 'Embedded xrdp server does not export xrdp_ohos_server_main: %s\n' "$embedded_server" >&2
    exit 1
  }
  grep -q ' T xrdp_ohos_server_stop' "$embedded_symbols" || {
    printf 'Embedded xrdp server does not export xrdp_ohos_server_stop: %s\n' "$embedded_server" >&2
    exit 1
  }
  grep -q 'lib=libxrdpohos.so' "$config" || {
    printf 'Installed xrdp.ini does not point to libxrdpohos.so: %s\n' "$config" >&2
    exit 1
  }
  grep -q '^rdpecam=true$' "$config" || {
    printf 'Installed xrdp.ini does not enable rdpecam: %s\n' "$config" >&2
    exit 1
  }
  if is_enabled "$ENABLE_OPENH264"; then
    grep -Eq 'openh264[[:space:]]+yes' "$LOG_DIR/xrdp-configure.log" || {
      printf 'xrdp configure did not enable OpenH264; see %s\n' "$LOG_DIR/xrdp-configure.log" >&2
      exit 1
    }
    grep -Eq 'rfxcodec[[:space:]]+yes' "$LOG_DIR/xrdp-configure.log" || {
      printf 'xrdp configure did not enable rfxcodec, which gates the GFX/H.264 path; see %s\n' "$LOG_DIR/xrdp-configure.log" >&2
      exit 1
    }
  fi

  log "xrdp OHOS outputs"
  file "$server" || true
  file "$embedded_server" || true
  file "$backend" || true
  printf 'config: %s\n' "$config"
}

main() {
  require_tool make
  require_tool pkg-config
  require_tool file
  require_tool grep
  load_ohos_env
  ensure_inputs
  configure_ccache

  mkdir -p "$OUT_DIR" "$BUILD_DIR" "$LOG_DIR" "$PREFIX" "$CONFIG_DIR" "$VAR_DIR" "$RUN_DIR"

  if [[ "$FORCE_REBUILD" == "1" ]]; then
    safe_rm_rf "$BUILD_DIR"
    safe_rm_rf "$PREFIX"
    safe_rm_rf "$CONFIG_DIR"
    safe_rm_rf "$VAR_DIR"
    safe_rm_rf "$RUN_DIR"
    mkdir -p "$BUILD_DIR" "$PREFIX" "$CONFIG_DIR" "$VAR_DIR" "$RUN_DIR"
  fi

  bootstrap_xrdp
  configure_xrdp
  build_xrdp
  install_xrdp
  verify_outputs
}

main "$@"
