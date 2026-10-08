#!/usr/bin/env bash
# =============================================================================
# build_appimage.sh — Build an x86_64 Linux AppImage
#
# The image contains the program, Qt, QML modules, the FFmpeg multimedia
# plugin, the OpenMP runtime, translations, and settings.ini.
#
# Usage:
#   ./scripts/build_appimage.sh [release|debug]
#
# Defaults: release
#
# Environment:
#   QT_ROOT        Qt installation root (default: $HOME/Qt/6.12.0)
#   QT_LINUX       gcc_64 kit (default: $QT_ROOT/gcc_64)
#   JOBS           Parallel compile jobs (default: nproc)
#
# The packaging tools are downloaded once into build_appimage/tools.
# patchelf is required. A system patchelf is used when it is installed;
# otherwise the copy bundled inside linuxdeploy is used.
#
# The image runs on systems whose glibc is at least as new as the build
# machine. Keep the file in a writable directory: settings.ini and
# data/sessions are stored beside it. The microphone uses the host
# PulseAudio or PipeWire library.
#
# Output:
#   build_appimage/SpeechRateMeter2-<version>-x86_64.AppImage
# =============================================================================

set -euo pipefail

QT_ROOT="${QT_ROOT:-$HOME/Qt/6.12.0}"
QT_LINUX="${QT_LINUX:-$QT_ROOT/gcc_64}"
APP_TARGET="appspeech-rate-meter-2"
DESKTOP_ID="by.intoncore.SpeechRateMeter2"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="release"
JOBS="${JOBS:-}"
VERSION=""
BUILD_DIR=""
OUT_DIR=""
APPDIR=""
TOOLS_DIR=""
LINUXDEPLOY=""
FINAL_IMAGE=""

LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
LINUXDEPLOY_APPIMAGE_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage"

usage() {
    cat <<EOF
Usage: ./scripts/build_appimage.sh [release|debug]

Builds an x86_64 AppImage with Qt, QML modules, the FFmpeg multimedia
plugin, the OpenMP runtime, and settings.ini.

Output:
  build_appimage/SpeechRateMeter2-<version>-x86_64.AppImage

See the header of scripts/build_appimage.sh for environment variables.
EOF
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

read_version() {
    VERSION="$(sed -n 's/^project(.*VERSION[[:space:]]\([0-9][.0-9]*\).*/\1/p' "$PROJECT_ROOT/CMakeLists.txt" | head -1)"
    [[ -n "$VERSION" ]] || VERSION="0"
}

check_host() {
    [[ "$(uname -s)" == "Linux" ]] || die "This script builds a Linux AppImage and must run on Linux."
    [[ "$(uname -m)" == "x86_64" ]] || die "This script builds an x86_64 AppImage. This machine is $(uname -m)."
    command -v cmake >/dev/null 2>&1 || die "cmake is required"
    command -v g++ >/dev/null 2>&1 || die "g++ is required"
    command -v curl >/dev/null 2>&1 || die "curl is required to download linuxdeploy"
    [[ -n "$JOBS" ]] || JOBS="$(nproc)"
}

check_qt() {
    [[ -x "$QT_LINUX/bin/qmake" ]] || die "Qt gcc_64 kit not found at $QT_LINUX. Set QT_LINUX or QT_ROOT."
    local qt_ver
    qt_ver="$("$QT_LINUX/bin/qmake" -query QT_VERSION)"
    [[ -n "$qt_ver" ]] || die "qmake -query QT_VERSION failed for $QT_LINUX"
    local lowest
    lowest="$(printf '%s\n%s\n' "6.12.0" "$qt_ver" | sort -V | head -1)"
    [[ "$lowest" == "6.12.0" ]] || die "Qt $qt_ver at $QT_LINUX is older than 6.12.0"
    echo "Qt: $QT_LINUX ($qt_ver)"
}

check_sources() {
    [[ -f "$PROJECT_ROOT/3rdparty/SPTK/CMakeLists.txt" ]] || \
        die "3rdparty/SPTK is missing. Clone it as described in README.md."
    [[ -f "$PROJECT_ROOT/3rdparty/alglib-cpp/src/ap.cpp" ]] || \
        die "3rdparty/alglib-cpp is missing. Unpack ALGLIB as described in README.md."
    [[ -f "$PROJECT_ROOT/settings.ini" ]] || die "settings.ini is missing from the project root"
    [[ -f "$PROJECT_ROOT/packaging/$DESKTOP_ID.desktop" ]] || \
        die "packaging/$DESKTOP_ID.desktop is missing"
    [[ -f "$PROJECT_ROOT/res/icons/hicolor/256x256/apps/$DESKTOP_ID.png" ]] || \
        die "res/icons/hicolor/256x256/apps/$DESKTOP_ID.png is missing"
    read_version
}

configure_and_build() {
    local -a cmake_args=()
    local cmake_config
    cmake_config="$(printf '%s%s' "$(printf '%s' "${BUILD_TYPE:0:1}" | tr '[:lower:]' '[:upper:]')" "${BUILD_TYPE:1}")"
    BUILD_DIR="$OUT_DIR/build"

    cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE="$cmake_config"
        -DCMAKE_PREFIX_PATH="$QT_LINUX"
        -DCMAKE_INSTALL_PREFIX=/usr
        -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON
        -DSPTK_INSTALL=OFF
    )
    if command -v ninja >/dev/null 2>&1; then
        cmake_args+=(-G Ninja)
    fi

    echo ""
    echo "============================================================"
    echo "  Configuring ($BUILD_TYPE)"
    echo "  Qt:    $QT_LINUX"
    echo "  Build: $BUILD_DIR"
    echo "============================================================"

    env -u CMAKE_PREFIX_PATH cmake "${cmake_args[@]}"
    cmake --build "$BUILD_DIR" --target "$APP_TARGET" release_translations --parallel "$JOBS"
}

stage_appdir() {
    local qm
    APPDIR="$OUT_DIR/AppDir"
    rm -rf "$APPDIR"
    mkdir -p "$APPDIR"

    echo "Installing into $APPDIR..."
    DESTDIR="$APPDIR" cmake --install "$BUILD_DIR" --prefix /usr

    [[ -x "$APPDIR/usr/bin/$APP_TARGET" ]] || \
        die "Installed binary not found at $APPDIR/usr/bin/$APP_TARGET"

    install -m 644 "$PROJECT_ROOT/settings.ini" "$APPDIR/usr/bin/settings.ini"

    local found=0
    for qm in "$BUILD_DIR"/speech-rate-meter-2_*.qm; do
        [[ -f "$qm" ]] || continue
        install -m 644 "$qm" "$APPDIR/usr/bin/$(basename "$qm")"
        found=1
    done
    [[ "$found" == 1 ]] || die "No translation catalogs (speech-rate-meter-2_*.qm) were generated in $BUILD_DIR"

    local linked
    # Match the resolved "lib => path" line. ldd also prints "version not found"
    # against the system Qt when the kit is not on the runtime path.
    linked="$(LD_LIBRARY_PATH="$QT_LINUX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        ldd "$APPDIR/usr/bin/$APP_TARGET" | awk '/libQt6Core\.so\.6/ && /=>/ { print $3; exit }')"
    [[ -n "$linked" && "$linked" != "not" ]] || die "The binary is not linked to libQt6Core.so.6"
    case "$linked" in
        "$QT_LINUX"/*) ;;
        *) die "The binary is linked to $linked, expected the kit at $QT_LINUX" ;;
    esac
}

download_tool() {
    local dest="$1"
    local url="$2"
    if [[ -x "$dest" ]]; then
        return 0
    fi
    echo "Downloading $(basename "$dest")..."
    mkdir -p "$(dirname "$dest")"
    curl -fL --retry 3 --retry-delay 2 -o "$dest.partial" "$url"
    mv "$dest.partial" "$dest"
    chmod +x "$dest"
}

ensure_tools() {
    TOOLS_DIR="$OUT_DIR/tools"
    download_tool "$TOOLS_DIR/linuxdeploy-x86_64.AppImage" "$LINUXDEPLOY_URL"
    download_tool "$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage" "$LINUXDEPLOY_QT_URL"
    download_tool "$TOOLS_DIR/linuxdeploy-plugin-appimage-x86_64.AppImage" "$LINUXDEPLOY_APPIMAGE_URL"
    LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
}

ensure_patchelf() {
    if command -v patchelf >/dev/null 2>&1; then
        return 0
    fi
    echo "patchelf is not on PATH. Using the copy bundled with linuxdeploy..."
    local extract="$TOOLS_DIR/linuxdeploy-extract"
    rm -rf "$extract"
    mkdir -p "$extract"
    ( cd "$extract" && "$LINUXDEPLOY" --appimage-extract >/dev/null )
    local bundled
    bundled="$(find "$extract/squashfs-root" -type f -name patchelf -print -quit)"
    [[ -n "$bundled" ]] || die "patchelf was not found inside linuxdeploy. Install it with: sudo apt install patchelf"
    mkdir -p "$TOOLS_DIR/bin"
    cp -a "$bundled" "$TOOLS_DIR/bin/patchelf"
    chmod +x "$TOOLS_DIR/bin/patchelf"
    export PATH="$TOOLS_DIR/bin:$PATH"
}

run_linuxdeploy() {
    local output="${1:-}"
    local -a args=(
        --appdir "$APPDIR"
        --executable "$APPDIR/usr/bin/$APP_TARGET"
        --desktop-file "$APPDIR/usr/share/applications/$DESKTOP_ID.desktop"
        --icon-file "$PROJECT_ROOT/res/icons/hicolor/256x256/apps/$DESKTOP_ID.png"
        --custom-apprun "$PROJECT_ROOT/packaging/AppRun"
        --exclude-library 'libpulse.so.0'
        --plugin qt
    )
    if [[ -n "$output" ]]; then
        args+=(--output "$output")
    fi

    # linuxdeploy and its plugins are AppImages. Extract them when FUSE
    # cannot mount from this environment. qmlimportscanner lives in libexec.
    local log="$OUT_DIR/linuxdeploy${output:+-$output}.log"
    echo "linuxdeploy log: $log"
    if ! APPIMAGE_EXTRACT_AND_RUN=1 \
        QMAKE="$QT_LINUX/bin/qmake" \
        QML_SOURCES_PATHS="$PROJECT_ROOT/ui" \
        EXTRA_QT_MODULES="waylandcompositor" \
        EXTRA_PLATFORM_PLUGINS="libqwayland.so" \
        PATH="$QT_LINUX/bin:$QT_LINUX/libexec:$PATH" \
        LD_LIBRARY_PATH="$QT_LINUX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        "$LINUXDEPLOY" "${args[@]}" >"$log" 2>&1; then
        echo "linuxdeploy failed. Last lines of $log:" >&2
        tail -n 40 "$log" >&2
        exit 1
    fi
}

# Qt Multimedia links libpulse. The matching libpulsecommon and Pulse
# modules belong to the host audio server, so they stay on the system.
drop_host_pulse() {
    find "$APPDIR" \( \
        -name 'libpulse.so*' -o \
        -name 'libpulse-simple.so*' -o \
        -name 'libpulsecommon*' \
    \) \( -type f -o -type l \) -delete
    rm -rf "$APPDIR/usr/share/doc/libpulse0"
    if find "$APPDIR" -name 'libpulse.so*' -print -quit | grep -q .; then
        die "libpulse.so is still inside the AppDir"
    fi
}

ensure_libgomp() {
    local found src
    found="$(find "$APPDIR" \( -name 'libgomp.so.1' -o -name 'libgomp.so' \) -print -quit)"
    if [[ -n "$found" ]]; then
        return 0
    fi
    src="$(g++ -print-file-name=libgomp.so.1)"
    [[ -f "$src" ]] || die "OpenMP runtime libgomp.so.1 was not deployed and was not found next to g++"
    mkdir -p "$APPDIR/usr/lib"
    cp -a "$src" "$APPDIR/usr/lib/libgomp.so.1"
    echo "Bundled OpenMP runtime: $src"
}

require_tree() {
    local description="$1"
    local pattern="$2"
    local found
    found="$(find "$APPDIR" -path "$pattern" -print -quit)"
    [[ -n "$found" ]] || die "Package is missing $description"
}

verify_appdir() {
    require_tree "qt.conf" "*/usr/bin/qt.conf"
    require_tree "settings.ini" "*/usr/bin/settings.ini"
    require_tree "English translations" "*/usr/bin/speech-rate-meter-2_en.qm"
    require_tree "Russian translations" "*/usr/bin/speech-rate-meter-2_ru.qm"
    require_tree "xcb platform plugin" "*/plugins/platforms/libqxcb.so"
    require_tree "Wayland platform plugin" "*/plugins/platforms/libqwayland.so"
    require_tree "FFmpeg multimedia plugin" "*/plugins/multimedia/libffmpegmediaplugin.so"
    require_tree "QtQuick" "*/qml/QtQuick/qmldir"
    require_tree "QtQuick.Controls" "*/qml/QtQuick/Controls/qmldir"
    require_tree "QtQuick.Controls.Material" "*/qml/QtQuick/Controls/Material/qmldir"
    require_tree "QtQuick.Dialogs" "*/qml/QtQuick/Dialogs/qmldir"
    require_tree "QtQuick.Layouts" "*/qml/QtQuick/Layouts/qmldir"
    require_tree "QtQuick.Shapes" "*/qml/QtQuick/Shapes/qmldir"
    require_tree "QtQuick.Window" "*/qml/QtQuick/Window/qmldir"
    require_tree "FFmpeg libavcodec" "*/libavcodec.so*"
    require_tree "OpenMP runtime" "*/libgomp.so*"
    grep -q '^Prefix' "$APPDIR/usr/bin/qt.conf" || die "qt.conf has no Prefix entry"
}

package_appimage() {
    echo ""
    echo "============================================================"
    echo "  Deploying Qt and QML into the AppDir"
    echo "============================================================"
    run_linuxdeploy
    # linuxdeploy may replace AppRun with a symlink. The shell launcher
    # execs the real binary so Qt finds qt.conf next to it.
    rm -f "$APPDIR/AppRun"
    install -m 755 "$PROJECT_ROOT/packaging/AppRun" "$APPDIR/AppRun"
    drop_host_pulse
    ensure_libgomp
    verify_appdir

    echo ""
    echo "============================================================"
    echo "  Building the AppImage"
    echo "============================================================"
    find "$OUT_DIR" -maxdepth 1 -name '*.AppImage' -delete
    local log="$OUT_DIR/linuxdeploy-appimage.log"
    local marker="$OUT_DIR/.packaging-stamp"
    touch "$marker"
    echo "linuxdeploy log: $log"
    # appimagetool writes the image into the current directory.
    if ! ( cd "$OUT_DIR" && APPIMAGE_EXTRACT_AND_RUN=1 \
        "$TOOLS_DIR/linuxdeploy-plugin-appimage-x86_64.AppImage" \
        --appdir "$APPDIR" >"$log" 2>&1 ); then
        echo "appimagetool failed. Last lines of $log:" >&2
        tail -n 40 "$log" >&2
        exit 1
    fi

    local produced
    produced="$(find "$OUT_DIR" -maxdepth 1 -name '*.AppImage' -newer "$marker" -print -quit)"
    [[ -n "$produced" ]] || die "linuxdeploy did not write an AppImage"

    FINAL_IMAGE="$OUT_DIR/SpeechRateMeter2-${VERSION}-x86_64.AppImage"
    if [[ "$produced" != "$FINAL_IMAGE" ]]; then
        mv -f "$produced" "$FINAL_IMAGE"
    fi
    chmod +x "$FINAL_IMAGE"
}

main() {
    case "${1:-release}" in
        release|debug) BUILD_TYPE="$1" ;;
        -h|--help|help) usage; exit 0 ;;
        *)
            usage >&2
            die "Unknown build type: $1"
            ;;
    esac

    OUT_DIR="$PROJECT_ROOT/build_appimage"
    mkdir -p "$OUT_DIR"

    check_host
    check_qt
    check_sources
    ensure_tools
    ensure_patchelf
    configure_and_build
    stage_appdir
    package_appimage

    echo ""
    echo "AppImage: $FINAL_IMAGE"
    echo "Keep this file in a writable directory. settings.ini and data/sessions are stored beside it."
}

main "$@"
