#!/usr/bin/env bash
# =============================================================================
# build_windows.sh — Build a portable 64-bit Windows package
#
# The zip contains SpeechRateMeter2.exe, the Qt runtime, QML modules,
# multimedia plugins, the OpenMP runtime, compiler runtime libraries,
# settings.ini, and the license texts.
#
# Usage:
#   ./scripts/build_windows.sh [release|debug]
#
# Defaults: release
#
# Environment:
#   QT_ROOT               Qt installation root (default: $HOME/Qt/6.12.0)
#   QT_WINDOWS            Windows kit directory
#                         (default: $QT_ROOT/mingw_64, or msvc2022_64)
#   QT_HOST_PATH          Linux Qt used for moc / qmlcachegen when
#                         cross-compiling (default: $QT_ROOT/gcc_64)
#   WINDOWS_TOOLCHAIN     mingw | msvc   (default: mingw on Linux;
#                         MSVC on Windows when that kit and Visual Studio
#                         are both installed)
#   JOBS                  Parallel compile jobs
#   ALLOW_MINGW_MISMATCH  Set to 1 to allow a MinGW major version other than 13
#
# Linux cross build needs:
#   - Qt 6.12 MinGW 13.1 64-bit kit at $QT_ROOT/mingw_64
#     (Qt Maintenance Tool component "MinGW 13.1.0 64-bit")
#   - g++-mingw-w64-x86-64 and binutils-mingw-w64-x86-64 (GCC 13, posix, seh)
#   - wine (windeployqt.exe is a Windows program)
#
# Windows build needs Git Bash and either:
#   - Qt MSVC 2022 64-bit + Visual Studio 2022, or
#   - Qt MinGW 13.1 64-bit and its matching MinGW under Qt/Tools
#
# Extract the zip to a writable folder. settings.ini and data/sessions
# are stored next to the executable.
# =============================================================================

set -euo pipefail

QT_ROOT="${QT_ROOT:-$HOME/Qt/6.12.0}"
APP_TARGET="appspeech-rate-meter-2"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="release"
HOST=""
TOOLCHAIN=""
CROSS=0
MULTI_CONFIG=0
QT_WINDOWS=""
QT_HOST=""
CXX=""
CC=""
WINDRES=""
JOBS="${JOBS:-}"
VERSION=""
BUILD_DIR=""
PACKAGE_DIR=""
ZIP_PATH=""
CMAKE_CONFIG=""
WINDEPLOYQT=""
WINE_SYSTEM32=""
WIN_SYSTEM32=""
PYTHON=()
SEARCH_DIRS=()
declare -A SYSTEM_DLL=()

usage() {
    cat <<EOF
Usage: ./scripts/build_windows.sh [release|debug]

Builds a 64-bit Windows folder with Qt, QML modules, multimedia plugins,
the OpenMP runtime, and settings.ini, then zips it.

Output:
  build_windows/SpeechRateMeter2-<version>-win64.zip

See the header of scripts/build_windows.sh for environment variables
and toolchain requirements.
EOF
}

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

load_system_dlls() {
    local name
    while IFS= read -r name; do
        [[ -n "$name" && "$name" != \#* ]] || continue
        SYSTEM_DLL["$name"]=1
    done <<'EOF'
kernel32.dll
kernelbase.dll
ntdll.dll
user32.dll
gdi32.dll
gdiplus.dll
shell32.dll
ole32.dll
oleaut32.dll
advapi32.dll
comctl32.dll
comdlg32.dll
imm32.dll
winmm.dll
ws2_32.dll
wsock32.dll
crypt32.dll
iphlpapi.dll
netapi32.dll
userenv.dll
version.dll
winspool.drv
winspool.dll
wtsapi32.dll
setupapi.dll
uxtheme.dll
dwmapi.dll
shlwapi.dll
shcore.dll
mpr.dll
dnsapi.dll
hid.dll
opengl32.dll
glu32.dll
msvcrt.dll
ucrtbase.dll
combase.dll
rpcrt4.dll
secur32.dll
sspicli.dll
bcrypt.dll
bcryptprimitives.dll
ncrypt.dll
cryptbase.dll
cryptsp.dll
cfgmgr32.dll
powrprof.dll
propsys.dll
avrt.dll
mfplat.dll
mf.dll
mfreadwrite.dll
mfcore.dll
evr.dll
d3d11.dll
d3d12.dll
d3d9.dll
dxgi.dll
dxva2.dll
dcomp.dll
ddraw.dll
dsound.dll
dinput8.dll
mmdevapi.dll
ksuser.dll
msdmo.dll
wmvcore.dll
quartz.dll
wintrust.dll
psapi.dll
normaliz.dll
sechost.dll
authz.dll
credui.dll
winhttp.dll
wininet.dll
urlmon.dll
oleacc.dll
usp10.dll
msimg32.dll
imagehlp.dll
dbghelp.dll
wldap32.dll
nsi.dll
dhcpcsvc.dll
dhcpcsvc6.dll
wevtapi.dll
faultrep.dll
wer.dll
cabinet.dll
mlang.dll
clbcatq.dll
msctf.dll
textshaping.dll
windows.storage.dll
coremessaging.dll
uiautomationcore.dll
avicap32.dll
msvfw32.dll
mag.dll
pdh.dll
msi.dll
esent.dll
sensapi.dll
rasapi32.dll
mswsock.dll
samcli.dll
wkscli.dll
netutils.dll
logoncli.dll
schedcli.dll
ntmarta.dll
dpapi.dll
rsaenh.dll
gpapi.dll
profapi.dll
kernel.appcore.dll
twinapi.appcore.dll
inputhost.dll
wintypes.dll
fwpuclnt.dll
ondemandconnroutehelper.dll
mscoree.dll
mshtml.dll
EOF
}

is_redistributable_runtime() {
    local name="${1,,}"
    case "$name" in
        libgcc_s_seh-1.dll|libstdc++-6.dll|libwinpthread-1.dll|libgomp-1.dll|libssp-0.dll|libatomic-1.dll|d3dcompiler_47.dll|opengl32sw.dll)
            return 0
            ;;
        vcruntime*.dll|msvcp140*.dll|vcomp140*.dll|concrt140*.dll|vcamp140*.dll|vccorlib140*.dll)
            return 0
            ;;
    esac
    return 1
}

is_optional_dll() {
    case "${1,,}" in
        d3dcompiler_47.dll|opengl32sw.dll) return 0 ;;
    esac
    return 1
}

is_system_dll() {
    local name="${1,,}"
    if is_redistributable_runtime "$name"; then
        return 1
    fi
    [[ "$name" == api-ms-* || "$name" == ext-ms-* ]] && return 0
    [[ -n "${SYSTEM_DLL[$name]+x}" ]] && return 0
    if [[ -n "${WINE_SYSTEM32:-}" && -f "$WINE_SYSTEM32/$name" ]]; then
        return 0
    fi
    if [[ -n "${WIN_SYSTEM32:-}" && -f "$WIN_SYSTEM32/$name" ]]; then
        return 0
    fi
    return 1
}

to_cmake_path() {
    if [[ "$HOST" == "windows" ]] && command -v cygpath >/dev/null 2>&1; then
        cygpath -m "$1" | tr -d '\r'
    else
        printf '%s\n' "$1"
    fi
}

to_native_path() {
    if [[ "$HOST" == "windows" ]]; then
        if command -v cygpath >/dev/null 2>&1; then
            cygpath -w "$1" | tr -d '\r'
        else
            printf '%s\n' "$1"
        fi
    else
        # /usr/bin/wine prefers the 32-bit loader when both are installed.
        "$WINE64" winepath -w "$1" | tr -d '\r'
    fi
}

qt_version() {
    local config line
    for config in \
        "$1/lib/cmake/Qt6/Qt6ConfigVersionImpl.cmake" \
        "$1/lib/cmake/Qt6/Qt6ConfigVersion.cmake"
    do
        [[ -f "$config" ]] || continue
        line="$(sed -n 's/^set(PACKAGE_VERSION[[:space:]]*"\{0,1\}\([0-9][.0-9]*\)"\{0,1\}).*/\1/p' "$config")"
        line="${line%%$'\n'*}"
        if [[ -n "$line" ]]; then
            printf '%s\n' "$line"
            return 0
        fi
    done
    return 1
}

find_python() {
    if command -v python3 >/dev/null 2>&1; then
        PYTHON=(python3)
    elif command -v py >/dev/null 2>&1; then
        PYTHON=(py -3)
    elif command -v python >/dev/null 2>&1; then
        PYTHON=(python)
    else
        die "python3 is required to scan PE imports in the Windows package"
    fi
}

abs_path() {
    local path="$1"
    if command -v readlink >/dev/null 2>&1 && readlink -f "$path" >/dev/null 2>&1; then
        readlink -f "$path"
    else
        printf '%s\n' "$path"
    fi
}

linux_prereq_help() {
    cat <<EOF
Linux cross builds need three things:

  1. Qt $(basename "$QT_ROOT") MinGW 64-bit kit, installed with Qt Maintenance Tool
     (component "MinGW 13.1.0 64-bit"). Expected directory:
       $QT_ROOT/mingw_64
     Override with QT_WINDOWS if the kit lives somewhere else.
     The matching Linux kit must be at:
       ${QT_HOST_PATH:-$QT_ROOT/gcc_64}

  2. GCC 13 MinGW cross compiler, posix threads, SEH exceptions:
       sudo apt install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64
       sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
       sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix

  3. 64-bit Wine, used to run windeployqt.exe (the 32-bit wine package cannot):
       sudo apt install wine64
EOF
}

resolve_jobs() {
    if [[ -n "$JOBS" ]]; then
        return 0
    fi
    if command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    else
        JOBS="${NUMBER_OF_PROCESSORS:-4}"
    fi
}

detect_host() {
    case "$(uname -s)" in
        Linux*) HOST=linux ;;
        MINGW*|MSYS*|CYGWIN*) HOST=windows ;;
        *) die "Unsupported host '$(uname -s)'. Run this script on Linux or in Git Bash on Windows." ;;
    esac
    if [[ "$HOST" == "windows" ]]; then
        local root="${SYSTEMROOT:-C:/Windows}"
        if command -v cygpath >/dev/null 2>&1; then
            WIN_SYSTEM32="$(cygpath -u "$root/System32" | tr -d '\r')"
        else
            WIN_SYSTEM32="/c/Windows/System32"
        fi
    fi
}

find_vs_install() {
    local vswhere="" candidate win_path
    if command -v vswhere >/dev/null 2>&1; then
        vswhere="$(command -v vswhere)"
    fi
    for candidate in \
        "$vswhere" \
        "/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe" \
        "/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
            vswhere="$candidate"
            break
        fi
    done
    [[ -n "$vswhere" && -x "$vswhere" ]] || return 1
    win_path="$("$vswhere" -latest -products '*' \
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
        -property installationPath | tr -d '\r' || true)"
    [[ -n "$win_path" ]] || return 1
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -u "$win_path" | tr -d '\r'
    else
        printf '%s\n' "$win_path"
    fi
}

resolve_toolchain() {
    if [[ -n "${QT_WINDOWS:-}" && -z "${WINDOWS_TOOLCHAIN:-}" ]]; then
        case "$QT_WINDOWS" in
            *msvc*) WINDOWS_TOOLCHAIN=msvc ;;
            *) WINDOWS_TOOLCHAIN=mingw ;;
        esac
    fi
    if [[ -z "${WINDOWS_TOOLCHAIN:-}" ]]; then
        if [[ "$HOST" == "linux" ]]; then
            WINDOWS_TOOLCHAIN=mingw
        elif [[ -d "$QT_ROOT/msvc2022_64" ]] && find_vs_install >/dev/null; then
            WINDOWS_TOOLCHAIN=msvc
        else
            WINDOWS_TOOLCHAIN=mingw
        fi
    fi
    TOOLCHAIN="$WINDOWS_TOOLCHAIN"
    case "$TOOLCHAIN" in
        mingw|msvc) ;;
        *) die "WINDOWS_TOOLCHAIN must be 'mingw' or 'msvc' (got '$TOOLCHAIN')" ;;
    esac
    if [[ "$HOST" == "linux" && "$TOOLCHAIN" == "msvc" ]]; then
        die "The MSVC kit cannot be cross-compiled from Linux. Install the MinGW kit or run this script on Windows."
    fi
    if [[ -z "${QT_WINDOWS:-}" ]]; then
        if [[ "$TOOLCHAIN" == "msvc" ]]; then
            QT_WINDOWS="$QT_ROOT/msvc2022_64"
        else
            QT_WINDOWS="$QT_ROOT/mingw_64"
        fi
    fi
    QT_HOST="${QT_HOST_PATH:-$QT_ROOT/gcc_64}"

    if [[ "$TOOLCHAIN" == "mingw" ]]; then
        if [[ "$HOST" == "linux" ]]; then
            CXX="${MINGW_CXX:-x86_64-w64-mingw32-g++}"
            if ! command -v "$CXX" >/dev/null 2>&1 || [[ ! -d "$QT_WINDOWS" ]]; then
                linux_prereq_help >&2
                if ! command -v "$CXX" >/dev/null 2>&1; then
                    die "MinGW cross compiler not found ('$CXX')"
                fi
                die "Qt MinGW kit not found at '$QT_WINDOWS'"
            fi
            require_wine64
            CROSS=1
        else
            CXX="$(find_windows_mingw)" || die "MinGW g++ was not found. Install Qt's MinGW 13.1 kit or set MINGW_CXX."
        fi
        CXX="$(abs_path "$(command -v "$CXX")")"
        CC="${CXX/g++/gcc}"
        # readlink resolves g++ to g++-posix. windres has no thread-model suffix.
        WINDRES="$(mingw_tool windres)" || die "windres was not found next to $CXX"
        [[ -x "$CC" ]] || die "MinGW C compiler not found at $CC"
        [[ -x "$WINDRES" ]] || die "windres not found at $WINDRES"
        check_mingw_compiler
        export PATH="$(dirname "$CXX"):$PATH"
    fi

    [[ -d "$QT_WINDOWS" ]] || \
        die "Qt Windows kit not found at '$QT_WINDOWS'. Install the Qt 6.12 ${TOOLCHAIN} 64-bit kit with Qt Maintenance Tool."
    [[ -f "$QT_WINDOWS/bin/windeployqt.exe" || -f "$QT_WINDOWS/bin/windeployqt" ]] || \
        die "windeployqt not found in $QT_WINDOWS/bin"
    if [[ -f "$QT_WINDOWS/bin/windeployqt.exe" ]]; then
        WINDEPLOYQT="$QT_WINDOWS/bin/windeployqt.exe"
    else
        WINDEPLOYQT="$QT_WINDOWS/bin/windeployqt"
    fi
}

# Tool name beside the MinGW C++ compiler. g++ may be the posix binary
# (x86_64-w64-mingw32-g++-posix); windres stays x86_64-w64-mingw32-windres.
mingw_tool() {
    local tool="$1"
    local dir base triplet candidate
    dir="$(dirname "$CXX")"
    base="$(basename "$CXX")"
    if [[ "$base" == *-g++* ]]; then
        triplet="${base%%-g++*}"
        for candidate in "$dir/${triplet}-${tool}" "$dir/${triplet}-${tool}.exe"; do
            if [[ -x "$candidate" ]]; then
                printf '%s\n' "$candidate"
                return 0
            fi
        done
        if command -v "${triplet}-${tool}" >/dev/null 2>&1; then
            command -v "${triplet}-${tool}"
            return 0
        fi
    fi
    candidate="${CXX/g++/$tool}"
    if [[ -x "$candidate" ]]; then
        printf '%s\n' "$candidate"
        return 0
    fi
    return 1
}

find_windows_mingw() {
    local dir pattern
    local -a kits=()
    if [[ -n "${MINGW_CXX:-}" ]]; then
        printf '%s\n' "$MINGW_CXX"
        return 0
    fi
    if [[ -n "${MINGW_ROOT:-}" ]]; then
        kits+=("$MINGW_ROOT")
    fi
    if [[ -d "$QT_ROOT/../Tools" ]]; then
        while IFS= read -r dir; do
            kits+=("$dir")
        done < <(find "$QT_ROOT/../Tools" -maxdepth 2 -type d -name 'mingw*_64' 2>/dev/null || true)
    fi
    if ((${#kits[@]} > 0)); then
        for dir in "${kits[@]}"; do
            for pattern in "$dir/bin/g++.exe" "$dir/bin/g++" "$dir/g++.exe"; do
                if [[ -x "$pattern" ]]; then
                    printf '%s\n' "$pattern"
                    return 0
                fi
            done
        done
    fi
    if command -v g++ >/dev/null 2>&1; then
        local machine
        machine="$(g++ -dumpmachine 2>/dev/null || true)"
        if [[ "$machine" == *mingw* ]]; then
            command -v g++
            return 0
        fi
    fi
    return 1
}

check_mingw_compiler() {
    local version major thread_model
    version="$("$CXX" -dumpversion)"
    # Debian's MinGW reports "13-posix" rather than "13.2.0".
    major="${version%%[^0-9]*}"
    thread_model="$("$CXX" -v 2>&1 | sed -n 's/^Thread model:[[:space:]]*//p' || true)"
    thread_model="${thread_model%%$'\n'*}"
    if [[ "$thread_model" == "win32" ]]; then
        die "MinGW is using the win32 thread model. Qt needs posix threads. On Debian/Ubuntu: sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix && sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix"
    fi
    if [[ -n "$thread_model" && "$thread_model" != "posix" ]]; then
        echo "WARNING: MinGW thread model is '$thread_model' (Qt expects posix)."
    fi
    local defines
    defines="$(printf '\n' | "$CXX" -dM -E - || true)"
    case "$defines" in
        *__SEH__*) ;;
        *) echo "WARNING: MinGW is not using SEH exceptions. Qt 6.12 MinGW builds use posix/seh." ;;
    esac
    if [[ "$major" != "13" && "${ALLOW_MINGW_MISMATCH:-}" != "1" ]]; then
        die "MinGW $version does not match Qt 6.12 (GCC 13.1 posix/seh). Install the GCC 13 cross compiler, or set ALLOW_MINGW_MISMATCH=1 to continue anyway."
    fi
    if [[ "$major" == "13" && "$version" != "13.1.0" && "$version" != "13.1" ]]; then
        echo "WARNING: Qt 6.12 MinGW builds use GCC 13.1. This compiler is $version."
        echo "         The package will ship this compiler's runtime DLLs."
    fi
}

ensure_ninja() {
    if command -v ninja >/dev/null 2>&1; then
        return 0
    fi
    local dir="$QT_ROOT/../Tools/Ninja"
    if [[ -x "$dir/ninja" || -x "$dir/ninja.exe" ]]; then
        export PATH="$dir:$PATH"
    fi
    command -v ninja >/dev/null 2>&1 || die "ninja is required for the MinGW build"
}

check_sources() {
    [[ -f "$PROJECT_ROOT/3rdparty/SPTK/CMakeLists.txt" ]] || \
        die "3rdparty/SPTK is missing. Clone it as described in README.md."
    [[ -f "$PROJECT_ROOT/3rdparty/alglib-cpp/src/ap.cpp" ]] || \
        die "3rdparty/alglib-cpp is missing. Unpack ALGLIB as described in README.md."
    [[ -f "$PROJECT_ROOT/settings.ini" ]] || die "settings.ini is missing from the project root"
    [[ -f "$PROJECT_ROOT/res/icons/speechratemeter.ico" ]] || \
        die "res/icons/speechratemeter.ico is missing"
    if [[ ! -f "$PROJECT_ROOT/res/icons/speechratemeter.rc" ]]; then
        printf 'IDI_ICON1 ICON "speechratemeter.ico"\n' > "$PROJECT_ROOT/res/icons/speechratemeter.rc"
    fi
    VERSION="$(sed -n 's/^project(.*VERSION[[:space:]]\([0-9][.0-9]*\).*/\1/p' "$PROJECT_ROOT/CMakeLists.txt" | head -1)"
    [[ -n "$VERSION" ]] || VERSION="0"
}

configure_and_build() {
    local -a cmake_args=()
    local target_ver host_ver rc_include
    command -v cmake >/dev/null 2>&1 || die "cmake is required"
    target_ver="$(qt_version "$QT_WINDOWS")" || die "Not a Qt kit: $QT_WINDOWS"
    rc_include="$(to_cmake_path "$PROJECT_ROOT/res/icons")"
    BUILD_DIR="$PROJECT_ROOT/build_windows"
    mkdir -p "$BUILD_DIR"
    # qmlcachegen treats the build directory as a QML import path. A Wine
    # prefix there contains dosdevices/z: -> /, so the scanner walks the
    # whole filesystem. Keep Wine outside the build tree.
    if [[ -d "$BUILD_DIR/wineprefix" ]]; then
        rm -rf "$BUILD_DIR/wineprefix"
    fi

    cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -DCMAKE_PREFIX_PATH="$(to_cmake_path "$QT_WINDOWS")"
        -DQT_NO_QTPATHS_DEPLOYMENT_WARNING=ON
    )

    if [[ "$TOOLCHAIN" == "msvc" ]]; then
        MULTI_CONFIG=1
        CMAKE_CONFIG="$(printf '%s%s' "$(printf '%s' "${BUILD_TYPE:0:1}" | tr '[:lower:]' '[:upper:]')" "${BUILD_TYPE:1}")"
        cmake_args+=(
            -G "Visual Studio 17 2022"
            -A x64
            "-DCMAKE_RC_FLAGS=/I${rc_include}"
        )
    else
        ensure_ninja
        CMAKE_CONFIG="$(printf '%s%s' "$(printf '%s' "${BUILD_TYPE:0:1}" | tr '[:lower:]' '[:upper:]')" "${BUILD_TYPE:1}")"
        cmake_args+=(
            -G Ninja
            -DCMAKE_BUILD_TYPE="$CMAKE_CONFIG"
            -DCMAKE_C_COMPILER="$(to_cmake_path "$CC")"
            -DCMAKE_CXX_COMPILER="$(to_cmake_path "$CXX")"
            -DCMAKE_RC_COMPILER="$(to_cmake_path "$WINDRES")"
            "-DCMAKE_RC_FLAGS=-I${rc_include}"
        )
        if [[ "$CROSS" == 1 ]]; then
            host_ver="$(qt_version "$QT_HOST")" || \
                die "Host Qt not found at $QT_HOST. Cross builds need the matching gcc_64 kit for moc and qmlcachegen."
            [[ "$host_ver" == "$target_ver" ]] || \
                die "Host Qt $host_ver and Windows Qt $target_ver differ. Use the same Qt version for both kits."
            cat > "$BUILD_DIR/mingw-toolchain.cmake" <<EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_C_COMPILER "$(to_cmake_path "$CC")")
set(CMAKE_CXX_COMPILER "$(to_cmake_path "$CXX")")
set(CMAKE_RC_COMPILER "$(to_cmake_path "$WINDRES")")
set(CMAKE_RC_FLAGS "-I${rc_include}")
set(CMAKE_FIND_ROOT_PATH "$(to_cmake_path "$QT_WINDOWS")")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
EOF
            cmake_args+=(
                -DCMAKE_TOOLCHAIN_FILE="$(to_cmake_path "$BUILD_DIR/mingw-toolchain.cmake")"
                -DQT_HOST_PATH="$(to_cmake_path "$QT_HOST")"
                -DQT_HOST_PATH_CMAKE_DIR="$(to_cmake_path "$QT_HOST/lib/cmake")"
                -DQT_REQUIRE_HOST_PATH_CHECK=ON
            )
            local pkg
            for pkg in Qt6HostInfo Qt6CoreTools Qt6GuiTools Qt6QmlTools Qt6QuickTools Qt6LinguistTools Qt6WidgetsTools; do
                if [[ -d "$QT_HOST/lib/cmake/$pkg" ]]; then
                    cmake_args+=("-D${pkg}_DIR=$(to_cmake_path "$QT_HOST/lib/cmake/$pkg")")
                fi
            done
        fi
    fi

    echo ""
    echo "============================================================"
    echo "  Configuring ($TOOLCHAIN, $BUILD_TYPE)"
    echo "  Qt:     $QT_WINDOWS ($target_ver)"
    if [[ "$CROSS" == 1 ]]; then
        echo "  Host:   $QT_HOST"
        echo "  Compiler: $CXX"
    fi
    echo "  Build:  $BUILD_DIR"
    echo "============================================================"

    if [[ "$CROSS" == 1 ]]; then
        env -u CMAKE_PREFIX_PATH -u QT_HOST_PATH PKG_CONFIG=/bin/false \
            cmake "${cmake_args[@]}"
    else
        env -u CMAKE_PREFIX_PATH -u QT_HOST_PATH \
            cmake "${cmake_args[@]}"
    fi

    local -a build_args=(--target "$APP_TARGET" --parallel "$JOBS")
    if [[ "$MULTI_CONFIG" == 1 ]]; then
        build_args+=(--config "$CMAKE_CONFIG")
    fi
    cmake --build "$BUILD_DIR" "${build_args[@]}"
}

built_exe() {
    local candidate
    for candidate in \
        "$BUILD_DIR/$CMAKE_CONFIG/$APP_TARGET.exe" \
        "$BUILD_DIR/$APP_TARGET.exe"
    do
        if [[ -f "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

# windeployqt.exe is a 64-bit PE. The wine32 package reports "Bad EXE format".
require_wine64() {
    local candidate
    for candidate in \
        "$(command -v wine64 2>/dev/null || true)" \
        /usr/lib/wine/wine64 \
        /usr/lib/x86_64-linux-gnu/wine/wine64
    do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
            WINE64="$candidate"
            return 0
        fi
    done
    linux_prereq_help >&2
    die "The 64-bit Wine loader was not found. Install it with: sudo apt install wine64"
}

ensure_wine() {
    [[ "$HOST" == "linux" ]] || return 0
    require_wine64
    # Outside the project. A prefix inside it has dosdevices/z: -> /, and
    # Qt's QML scanner follows that symlink across the whole filesystem.
    export WINEPREFIX="${XDG_CACHE_HOME:-$HOME/.cache}/SpeechRateMeter2/wine"
    export WINEDEBUG="${WINEDEBUG:--all}"
    export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
    WINE_SYSTEM32="$WINEPREFIX/drive_c/windows/system32"
    # A prefix created by wine32 has no syswow64 and cannot host 64-bit programs.
    if [[ -d "$WINEPREFIX" && ! -d "$WINEPREFIX/drive_c/windows/syswow64" ]]; then
        rm -rf "$WINEPREFIX"
    fi
    if [[ ! -d "$WINE_SYSTEM32" ]]; then
        mkdir -p "$WINEPREFIX"
        echo "Initializing a private 64-bit Wine prefix (first run only)..."
        if command -v timeout >/dev/null 2>&1; then
            timeout 120 "$WINE64" wineboot --init || true
        else
            "$WINE64" wineboot --init || true
        fi
    fi
    [[ -d "$WINE_SYSTEM32" ]] || die "Wine prefix was not created at $WINEPREFIX"
}

plugin_root() {
    if [[ -d "$PACKAGE_DIR/platforms" || -d "$PACKAGE_DIR/qml" ]]; then
        printf '%s\n' "$PACKAGE_DIR"
    elif [[ -d "$PACKAGE_DIR/plugins/platforms" ]]; then
        printf '%s\n' "$PACKAGE_DIR/plugins"
    else
        printf '%s\n' "$PACKAGE_DIR"
    fi
}

plugin_is_present() {
    local rel="$1" root
    root="$(plugin_root)"
    [[ -f "$root/$rel" || -f "$PACKAGE_DIR/$rel" || -f "$PACKAGE_DIR/plugins/$rel" ]]
}

ensure_plugin() {
    local rel="$1" root dest src
    if plugin_is_present "$rel"; then
        return 0
    fi
    src="$QT_WINDOWS/plugins/$rel"
    [[ -f "$src" ]] || die "Qt kit is missing plugins/$rel"
    root="$(plugin_root)"
    dest="$root/$rel"
    mkdir -p "$(dirname "$dest")"
    cp -a "$src" "$dest"
    echo "  plugin: $rel"
}

run_windeployqt() {
    local mode_flag="--release"
    local exe_native qml_native import_native dest_native log
    [[ "$BUILD_TYPE" == "debug" ]] && mode_flag="--debug"
    exe_native="$(to_native_path "$PACKAGE_DIR/SpeechRateMeter2.exe")"
    qml_native="$(to_native_path "$PROJECT_ROOT/ui")"
    import_native="$(to_native_path "$QT_WINDOWS/qml")"
    dest_native="$(to_native_path "$PACKAGE_DIR")"
    log="$BUILD_DIR/windeployqt.log"
    echo "Running windeployqt..."
    set +e
    if [[ "$HOST" == "linux" ]]; then
        "$WINE64" "$(to_native_path "$WINDEPLOYQT")" \
            "$mode_flag" \
            --compiler-runtime \
            --verbose 1 \
            --translations en,ru \
            --qmldir "$qml_native" \
            --qmlimport "$import_native" \
            --dir "$dest_native" \
            "$exe_native" >"$log" 2>&1
    else
        "$WINDEPLOYQT" \
            "$mode_flag" \
            --compiler-runtime \
            --verbose 1 \
            --translations en,ru \
            --qmldir "$qml_native" \
            --qmlimport "$import_native" \
            --dir "$dest_native" \
            "$exe_native" >"$log" 2>&1
    fi
    local status=$?
    set -e
    if [[ "$status" -ne 0 ]]; then
        echo "windeployqt failed (exit $status). Last lines of $log:" >&2
        tail -n 40 "$log" >&2 || true
        exit 1
    fi
    # Qt 6.12 windeployqt copies plugins next to the executable and does not
    # always write qt.conf. Without Plugins=., Qt looks in plugins/ and misses them.
    if [[ ! -f "$PACKAGE_DIR/qt.conf" ]]; then
        cat > "$PACKAGE_DIR/qt.conf" <<'EOF'
[Paths]
Prefix=.
Plugins=.
QmlImports=qml
Translations=translations
EOF
        echo "  wrote qt.conf"
    fi
}

copy_named_dlls_from_dir() {
    local dir="$1" label="$2" file base low
    [[ -d "$dir" ]] || return 0
    while IFS= read -r -d '' file; do
        base="$(basename "$file")"
        low="${base,,}"
        case "$low" in
            vcruntime*d.dll|msvcp*d.dll|vcomp*d.dll|concrt*d.dll|vcamp*d.dll|vccorlib*d.dll)
                [[ "$BUILD_TYPE" == "debug" ]] || continue
                ;;
        esac
        cp -a "$file" "$PACKAGE_DIR/"
        echo "  $label: $base"
    done < <(find "$dir" -maxdepth 1 -type f -iname '*.dll' -print0)
}

copy_msvc_runtime() {
    local vs crt omp
    vs="$(find_vs_install || true)"
    if [[ -z "$vs" ]]; then
        echo "WARNING: vswhere did not find Visual Studio. windeployqt must supply the VC runtime."
        return 0
    fi
    if [[ "$BUILD_TYPE" == "debug" ]]; then
        echo "WARNING: debug Visual C++ libraries are not redistributable."
    fi
    crt="$(find "$vs/VC/Redist/MSVC" -type d -path '*/x64/*' -name 'Microsoft.VC*.CRT' ! -ipath '*/onecore/*' ! -ipath '*/debug/*' | sort -V | tail -1 || true)"
    omp="$(find "$vs/VC/Redist/MSVC" -type d -path '*/x64/*' -name 'Microsoft.VC*.OpenMP' ! -ipath '*/onecore/*' ! -ipath '*/debug/*' | sort -V | tail -1 || true)"
    if [[ "$BUILD_TYPE" == "debug" ]]; then
        local debug_crt debug_omp
        debug_crt="$(find "$vs/VC" -type d -path '*/x64/*' -iname 'Microsoft.VC*.DebugCRT' | sort -V | tail -1 || true)"
        debug_omp="$(find "$vs/VC" -type d -path '*/x64/*' -iname 'Microsoft.VC*.DebugOpenMP' | sort -V | tail -1 || true)"
        [[ -n "$debug_crt" ]] && crt="$debug_crt"
        [[ -n "$debug_omp" ]] && omp="$debug_omp"
    fi
    [[ -n "$crt" ]] || die "MSVC CRT redistributable directory not found under $vs"
    [[ -n "$omp" ]] || die "MSVC OpenMP redistributable directory not found under $vs"
    copy_named_dlls_from_dir "$crt" "runtime"
    copy_named_dlls_from_dir "$omp" "runtime"
    SEARCH_DIRS+=("$crt" "$omp")
}

copy_one_runtime() {
    local name="$1" path="" dir
    path="$("$CXX" -print-file-name="$name" 2>/dev/null || true)"
    if [[ -z "$path" || "$path" == "$name" || ! -f "$path" ]]; then
        path=""
        for dir in "${SEARCH_DIRS[@]}"; do
            [[ -d "$dir" ]] || continue
            path="$(find "$dir" -iname "$name" -type f -print -quit 2>/dev/null || true)"
            [[ -n "$path" ]] && break
        done
    fi
    if [[ -n "$path" && -f "$path" ]]; then
        cp -a "$path" "$PACKAGE_DIR/$name"
        echo "  runtime: $name"
        return 0
    fi
    return 1
}

copy_mingw_runtime() {
    local name src
    # Qt's DLLs were built against the libstdc++ shipped in the MinGW kit.
    # The cross compiler's libstdc++ is a different build and breaks those imports.
    for name in libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll; do
        src="$QT_WINDOWS/bin/$name"
        if [[ -f "$src" ]]; then
            cp -a "$src" "$PACKAGE_DIR/$name"
            echo "  runtime: $name (Qt MinGW kit)"
        elif ! copy_one_runtime "$name"; then
            echo "WARNING: $name was not found in the Qt MinGW kit"
        fi
    done
    # OpenMP is not part of the Qt kit. libgomp from this GCC 13 builds
    # against the same libgcc/libstdc++ imports Qt's runtime provides.
    if ! copy_one_runtime "libgomp-1.dll"; then
        echo "WARNING: libgomp-1.dll was not found next to the compiler"
    fi
}

find_runtime_dll() {
    local name="$1" dir hit
    for dir in "${SEARCH_DIRS[@]}"; do
        [[ -d "$dir" ]] || continue
        hit="$(find "$dir" -iname "$name" -type f -print -quit 2>/dev/null || true)"
        if [[ -n "$hit" ]]; then
            printf '%s\n' "$hit"
            return 0
        fi
    done
    return 1
}

scan_package_imports() {
    "${PYTHON[@]}" - "$1" <<'PY'
import struct
import sys
from pathlib import Path

def pe_imports(path):
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        return []
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if e_lfanew <= 0 or e_lfanew + 24 > len(data) or data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
        return []
    coff = e_lfanew + 4
    _, nsec, _, _, _, optsize, _ = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    if opt + optsize > len(data):
        return []
    magic = struct.unpack_from("<H", data, opt)[0]
    if magic == 0x20B:
        dd_off = opt + 112
        nrva_off = opt + 108
    elif magic == 0x10B:
        dd_off = opt + 96
        nrva_off = opt + 92
    else:
        return []
    if nrva_off + 4 > len(data):
        return []
    nrva = struct.unpack_from("<I", data, nrva_off)[0]
    sec_off = opt + optsize
    sections = []
    for i in range(nsec):
        off = sec_off + i * 40
        if off + 24 > len(data):
            break
        vsz, va, rawsz, raw = struct.unpack_from("<IIII", data, off + 8)
        sections.append((va, max(vsz, rawsz), raw))

    def rva_to_off(rva):
        for va, span, raw in sections:
            if span and va <= rva < va + span:
                return raw + (rva - va)
        return None

    def read_cstr(off):
        end = data.find(b"\0", off)
        if end < 0:
            return ""
        return data[off:end].decode("ascii", "replace")

    names = []

    def add_dir(index, name_off, stride):
        if index >= nrva:
            return
        entry = dd_off + index * 8
        if entry + 8 > len(data):
            return
        rva, size = struct.unpack_from("<II", data, entry)
        if rva == 0 or size == 0:
            return
        off = rva_to_off(rva)
        if off is None:
            return
        while off + stride <= len(data):
            chunk = data[off:off + stride]
            if chunk == b"\0" * stride:
                break
            name_rva = struct.unpack_from("<I", data, off + name_off)[0]
            name_file = rva_to_off(name_rva) if name_rva else None
            if name_file is not None:
                nm = read_cstr(name_file).strip()
                if nm:
                    names.append(nm)
            off += stride

    add_dir(1, 12, 20)
    add_dir(13, 4, 32)
    return names

root = Path(sys.argv[1])
for pe in root.rglob("*"):
    if not pe.is_file() or pe.suffix.lower() not in {".dll", ".exe"}:
        continue
    for name in pe_imports(pe):
        print(name)
PY
}

refresh_present() {
    find "$PACKAGE_DIR" -type f -printf '%f\n' | tr '[:upper:]' '[:lower:]' | sort -u > "$BUILD_DIR/package-files.txt"
}

dll_present() {
    grep -Fxq "${1,,}" "$BUILD_DIR/package-files.txt"
}

fill_missing_dlls() {
    local pass=0 copied name src
    local missing="$BUILD_DIR/missing-dlls.txt"
    while (( pass < 8 )); do
        pass=$((pass + 1))
        refresh_present
        scan_package_imports "$PACKAGE_DIR" | tr -d '\r' | sort -u > "$BUILD_DIR/import-names.txt"
        : > "$missing"
        while IFS= read -r name; do
            [[ -n "$name" ]] || continue
            if is_system_dll "$name" || dll_present "$name"; then
                continue
            fi
            printf '%s\n' "$name" >> "$missing"
        done < "$BUILD_DIR/import-names.txt"
        if [[ ! -s "$missing" ]]; then
            return 0
        fi
        copied=0
        while IFS= read -r name; do
            src="$(find_runtime_dll "$name" || true)"
            if [[ -n "$src" ]]; then
                cp -a "$src" "$PACKAGE_DIR/"
                echo "  dependency: $(basename "$src")"
                copied=1
            fi
        done < "$missing"
        if (( copied == 0 )); then
            break
        fi
    done
    refresh_present
    local failed=0
    while IFS= read -r name; do
        [[ -n "$name" ]] || continue
        dll_present "$name" && continue
        if is_optional_dll "$name"; then
            echo "WARNING: optional DLL not bundled: $name"
            continue
        fi
        echo "ERROR: missing DLL: $name" >&2
        failed=1
    done < "$missing"
    [[ "$failed" -eq 0 ]]
}

require_root_dll() {
    local name="$1" found
    found="$(find "$PACKAGE_DIR" -maxdepth 1 -iname "$name" -type f -print -quit 2>/dev/null || true)"
    [[ -n "$found" ]] || die "Package is missing $name next to SpeechRateMeter2.exe"
}

require_qml() {
    local rel="$1"
    [[ -f "$PACKAGE_DIR/qml/$rel/qmldir" ]] || \
        die "Package is missing QML module $rel. See $BUILD_DIR/windeployqt.log"
}

write_package_readme() {
    local runtime_note
    if [[ "$TOOLCHAIN" == "mingw" ]]; then
        runtime_note="Qt is dynamically linked (LGPLv3). The MinGW and OpenMP runtime libraries are included."
    else
        runtime_note="Qt is dynamically linked (LGPLv3). The Visual C++ and OpenMP runtime libraries are included."
    fi
    cat > "$PACKAGE_DIR/README.txt" <<EOF
Speech Rate Meter 2
Version $VERSION

Run SpeechRateMeter2.exe in this folder.
Keep the folder writable: settings.ini and data/sessions are stored here.
Leave the DLL files and the qml, platforms, imageformats, and multimedia
folders next to the program.

$runtime_note
License texts are in LICENSE and the licenses folder.
Source availability is described in licenses/THIRD_PARTY_NOTICES.md.
EOF
}

make_zip() {
    local parent="$BUILD_DIR/package"
    rm -f "$ZIP_PATH"
    if command -v zip >/dev/null 2>&1; then
        (cd "$parent" && zip -r -q "$ZIP_PATH" "SpeechRateMeter2")
        return 0
    fi
    "${PYTHON[@]}" - "$ZIP_PATH" "$parent/SpeechRateMeter2" <<'PY'
import sys
import pathlib
import zipfile

dest = pathlib.Path(sys.argv[1])
src = pathlib.Path(sys.argv[2])
with zipfile.ZipFile(dest, "w", compression=zipfile.ZIP_DEFLATED) as archive:
    for path in src.rglob("*"):
        if path.is_file():
            archive.write(path, path.relative_to(src.parent).as_posix())
PY
}

package_app() {
    local exe suffix=""
    exe="$(built_exe)" || die "Build finished but $APP_TARGET.exe was not found in $BUILD_DIR"
    PACKAGE_DIR="$BUILD_DIR/package/SpeechRateMeter2"
    rm -rf "$BUILD_DIR/package"
    mkdir -p "$PACKAGE_DIR/licenses"
    cp -a "$exe" "$PACKAGE_DIR/SpeechRateMeter2.exe"
    cp -a "$PROJECT_ROOT/LICENSE" "$PACKAGE_DIR/LICENSE"
    cp -a "$PROJECT_ROOT/licenses/." "$PACKAGE_DIR/licenses/"
    cp -a "$PROJECT_ROOT/settings.ini" "$PACKAGE_DIR/settings.ini"
    write_package_readme

    SEARCH_DIRS=("$QT_WINDOWS/bin" "$QT_WINDOWS/plugins" "$QT_WINDOWS/lib")
    if [[ "$TOOLCHAIN" == "mingw" ]]; then
        local runtime_dll runtime_file
        SEARCH_DIRS+=("$(dirname "$CXX")")
        for runtime_dll in libgcc_s_seh-1.dll libwinpthread-1.dll libgomp-1.dll libstdc++-6.dll; do
            runtime_file="$("$CXX" -print-file-name="$runtime_dll" 2>/dev/null || true)"
            if [[ -f "$runtime_file" ]]; then
                SEARCH_DIRS+=("$(dirname "$runtime_file")")
            fi
        done
    fi

    ensure_wine
    run_windeployqt

    echo "Checking plugins..."
    [[ -f "$QT_WINDOWS/plugins/platforms/qwindows.dll" ]] || \
        die "Qt kit is missing plugins/platforms/qwindows.dll"
    # Qt 6.12 builds PNG into Qt6Gui. Older kits still ship qpng.dll.
    if [[ -f "$QT_WINDOWS/plugins/imageformats/qpng.dll" ]]; then
        ensure_plugin "imageformats/qpng.dll"
    fi
    if [[ ! -f "$QT_WINDOWS/plugins/multimedia/windowsmediaplugin.dll" ]]; then
        echo "Multimedia plugins in $QT_WINDOWS/plugins/multimedia:" >&2
        ls -1 "$QT_WINDOWS/plugins/multimedia" >&2 || true
        die "Qt kit is missing plugins/multimedia/windowsmediaplugin.dll (microphone capture needs it)"
    fi
    ensure_plugin "platforms/qwindows.dll"
    ensure_plugin "multimedia/windowsmediaplugin.dll"
    if [[ -f "$QT_WINDOWS/plugins/multimedia/ffmpegmediaplugin.dll" ]]; then
        ensure_plugin "multimedia/ffmpegmediaplugin.dll"
    fi
    if [[ -f "$QT_WINDOWS/plugins/styles/qmodernwindowsstyle.dll" ]]; then
        ensure_plugin "styles/qmodernwindowsstyle.dll"
    fi

    echo "Copying compiler and OpenMP runtimes..."
    if [[ "$TOOLCHAIN" == "msvc" ]]; then
        copy_msvc_runtime
    else
        copy_mingw_runtime
    fi

    echo "Copying remaining DLL dependencies..."
    fill_missing_dlls

    require_qml "QtQuick"
    require_qml "QtQuick/Controls"
    require_qml "QtQuick/Controls/Material"
    require_qml "QtQuick/Layouts"
    require_qml "QtQuick/Shapes"
    require_qml "QtQuick/Dialogs"
    require_qml "QtQuick/Window"
    if [[ "$TOOLCHAIN" == "mingw" ]]; then
        require_root_dll "libstdc++-6.dll"
        require_root_dll "libgcc_s_seh-1.dll"
        require_root_dll "libwinpthread-1.dll"
        require_root_dll "libgomp-1.dll"
    else
        local omp_name="vcomp140.dll" crt_name="vcruntime140.dll" cpp_name="msvcp140.dll"
        if [[ "$BUILD_TYPE" == "debug" ]]; then
            omp_name="vcomp140d.dll"
            crt_name="vcruntime140d.dll"
            cpp_name="msvcp140d.dll"
        fi
        require_root_dll "$crt_name"
        require_root_dll "$cpp_name"
        require_root_dll "$omp_name"
    fi
    [[ -f "$PACKAGE_DIR/settings.ini" ]] || die "settings.ini was not copied into the package"
    cp -a "$PROJECT_ROOT/settings.ini" "$PACKAGE_DIR/settings.ini"

    if [[ "$BUILD_TYPE" == "debug" ]]; then
        suffix="-debug"
    fi
    ZIP_PATH="$BUILD_DIR/SpeechRateMeter2-${VERSION}-win64${suffix}.zip"
    make_zip

    local dll_count
    dll_count="$(find "$PACKAGE_DIR" -iname '*.dll' | wc -l | tr -d ' ')"
    echo ""
    echo "============================================================"
    echo "  Windows package ready"
    echo "  Folder: $PACKAGE_DIR"
    echo "  Zip:    $ZIP_PATH"
    echo "  DLLs:   $dll_count"
    echo "============================================================"
    echo "Extract the zip to a writable directory and run SpeechRateMeter2.exe."
}

main() {
    local arg
    for arg in "$@"; do
        case "$arg" in
            -h|--help) usage; exit 0 ;;
            release|debug) BUILD_TYPE="$arg" ;;
            *) die "Unknown argument '$arg'. Usage: ./scripts/build_windows.sh [release|debug]" ;;
        esac
    done

    detect_host
    resolve_jobs
    find_python
    load_system_dlls
    check_sources
    resolve_toolchain

    echo "Host:        $HOST"
    echo "Toolchain:   $TOOLCHAIN"
    echo "Build type:  $BUILD_TYPE"
    echo "Qt Windows:  $QT_WINDOWS"
    echo "Jobs:        $JOBS"

    configure_and_build
    package_app
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
