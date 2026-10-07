#!/usr/bin/env bash
# =============================================================================
# build_android.sh — Build Speech Rate Meter 2 APK + Android App Bundle (.aab)
#
# Usage:
#   ./scripts/build_android.sh [arm64-v8a|armeabi-v7a|x86_64|all]  [debug|release]
#
# Defaults: ABI=arm64-v8a, BUILD_TYPE=release
#
# Signing (optional; required for a Play-uploadable AAB):
#   QT_ANDROID_KEYSTORE_PATH, QT_ANDROID_KEYSTORE_ALIAS,
#   QT_ANDROID_KEYSTORE_STORE_PASS, QT_ANDROID_KEYSTORE_KEY_PASS
#
# Aliases (copied into the QT_ANDROID_* vars if those are unset):
#   ANDROID_KEYSTORE_PATH, ANDROID_KEYSTORE_ALIAS, ANDROID_KEYSTORE_PASSWORD
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration — override any of these via environment variables
# ---------------------------------------------------------------------------
QT_ROOT="${QT_ROOT:-$HOME/Qt/6.12.0}"
ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
ANDROID_NDK="${ANDROID_NDK:-$ANDROID_SDK/ndk/27.2.12479018}"
# Android Gradle Plugin requires Java 17+
JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_TARGET="appspeech-rate-meter-2"

TARGET_ABI="${1:-arm64-v8a}"   # arm64-v8a | armeabi-v7a | x86_64 | all
BUILD_TYPE="${2:-release}"      # debug | release

# Convenience aliases → official Qt androiddeployqt env vars
if [[ -n "${ANDROID_KEYSTORE_PATH:-}" && -z "${QT_ANDROID_KEYSTORE_PATH:-}" ]]; then
    export QT_ANDROID_KEYSTORE_PATH="$ANDROID_KEYSTORE_PATH"
fi
if [[ -n "${ANDROID_KEYSTORE_ALIAS:-}" && -z "${QT_ANDROID_KEYSTORE_ALIAS:-}" ]]; then
    export QT_ANDROID_KEYSTORE_ALIAS="$ANDROID_KEYSTORE_ALIAS"
fi
if [[ -n "${ANDROID_KEYSTORE_PASSWORD:-}" ]]; then
    export QT_ANDROID_KEYSTORE_STORE_PASS="${QT_ANDROID_KEYSTORE_STORE_PASS:-$ANDROID_KEYSTORE_PASSWORD}"
    export QT_ANDROID_KEYSTORE_KEY_PASS="${QT_ANDROID_KEYSTORE_KEY_PASS:-$ANDROID_KEYSTORE_PASSWORD}"
fi

# Always pass ON/OFF so a previous CMake cache cannot leave --sign enabled
# without a keystore (androiddeployqt then fails: "signing path and alias
# values are not specified").
if [[ -n "${QT_ANDROID_KEYSTORE_PATH:-}" ]]; then
    if [[ ! -f "$QT_ANDROID_KEYSTORE_PATH" ]]; then
        echo "ERROR: keystore not found: $QT_ANDROID_KEYSTORE_PATH"
        exit 1
    fi
    if [[ -z "${QT_ANDROID_KEYSTORE_ALIAS:-}" ]]; then
        echo "ERROR: QT_ANDROID_KEYSTORE_ALIAS is required when signing is enabled"
        exit 1
    fi
    SIGN_CMAKE_ARGS=(-DQT_ANDROID_SIGN_AAB=ON -DQT_ANDROID_SIGN_APK=ON)
    echo "Signing: enabled ($QT_ANDROID_KEYSTORE_PATH)"
else
    SIGN_CMAKE_ARGS=(-DQT_ANDROID_SIGN_AAB=OFF -DQT_ANDROID_SIGN_APK=OFF)
    echo "Signing: disabled (set QT_ANDROID_KEYSTORE_PATH for a Play-uploadable AAB)"
fi

# ---------------------------------------------------------------------------
# Helper: build a single ABI
# ---------------------------------------------------------------------------
build_abi() {
    local ABI="$1"
    local QT_ANDROID_DIR
    local CMAKE_ABI="$ABI"

    case "$ABI" in
        arm64-v8a)  QT_ANDROID_DIR="$QT_ROOT/android_arm64_v8a" ;;
        armeabi-v7a|armv7)
            QT_ANDROID_DIR="$QT_ROOT/android_armv7"
            CMAKE_ABI="armeabi-v7a"
            ;;
        x86_64)     QT_ANDROID_DIR="$QT_ROOT/android_x86_64"    ;;
        *)
            echo "ERROR: unknown ABI '$ABI'. Valid: arm64-v8a armeabi-v7a x86_64"
            exit 1
            ;;
    esac

    if [[ ! -d "$QT_ANDROID_DIR" ]]; then
        echo "ERROR: Qt for Android not found at '$QT_ANDROID_DIR'"
        echo "       Install the '${ABI}' component via Qt Maintenance Tool."
        exit 1
    fi

    local BUILD_DIR="$PROJECT_ROOT/build_android_${CMAKE_ABI}"
    echo ""
    echo "============================================================"
    echo "  Building ABI: $CMAKE_ABI"
    echo "  Qt:  $QT_ANDROID_DIR"
    echo "  NDK: $ANDROID_NDK"
    echo "  Build dir: $BUILD_DIR"
    echo "============================================================"

    cmake \
        -S "$PROJECT_ROOT" \
        -B "$BUILD_DIR" \
        -G "Ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$CMAKE_ABI" \
        -DANDROID_PLATFORM="android-26" \
        -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
        -DANDROID_NDK="$ANDROID_NDK" \
        -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
        -DCMAKE_FIND_ROOT_PATH="$QT_ANDROID_DIR" \
        -DCMAKE_PREFIX_PATH="$QT_ANDROID_DIR" \
        -DQT_HOST_PATH="$QT_ROOT/gcc_64" \
        -DQT_HOST_PATH_CMAKE_DIR="$QT_ROOT/gcc_64/lib/cmake" \
        -DCMAKE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}" \
        -DANDROID_SDK_ROOT="$ANDROID_SDK" \
        "${SIGN_CMAKE_ARGS[@]}"

    cmake --build "$BUILD_DIR" --target "$APP_TARGET" -- -j"$(nproc)"

    echo ""
    echo "  ABI $CMAKE_ABI build complete. Artifacts in: $BUILD_DIR"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
export JAVA_HOME
export ANDROID_HOME="$ANDROID_SDK"
# Gradle picks up the JDK via GRADLE_OPTS (Android Gradle Plugin requires Java 17)
export GRADLE_OPTS="-Dorg.gradle.java.home=${JAVA_HOME}"

echo "Android SDK:   $ANDROID_SDK"
echo "Android NDK:   $ANDROID_NDK"
echo "Java home:     $JAVA_HOME"
echo "Build type:    $BUILD_TYPE"
echo "Target ABI(s): $TARGET_ABI"
echo ""

# Verify essential tools
for tool in cmake ninja java; do
    command -v "$tool" >/dev/null || { echo "ERROR: '$tool' not found in PATH"; exit 1; }
done

if [[ "$TARGET_ABI" == "all" ]]; then
    for abi in arm64-v8a armeabi-v7a x86_64; do
        build_abi "$abi"
    done
else
    build_abi "$TARGET_ABI"
fi

# ---------------------------------------------------------------------------
# Package APK (emulator / sideload) and AAB (Play) via Qt CMake targets
# ---------------------------------------------------------------------------
# Prerequisite: platforms;android-36 must be installed in the SDK:
#   JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64 \
#     $HOME/Android/Sdk/cmdline-tools/latest/bin/sdkmanager "platforms;android-36" <<< "y"
# ---------------------------------------------------------------------------
echo ""
echo "============================================================"
echo "  Packaging APK and Android App Bundle (.aab)"
echo "============================================================"

MAIN_ABI="$TARGET_ABI"
if [[ "$TARGET_ABI" == "all" ]]; then
    MAIN_ABI="arm64-v8a"
elif [[ "$TARGET_ABI" == "armv7" ]]; then
    MAIN_ABI="armeabi-v7a"
fi
BUILD_DIR="$PROJECT_ROOT/build_android_${MAIN_ABI}"

cmake --build "$BUILD_DIR" --target apk -- -j"$(nproc)"
cmake --build "$BUILD_DIR" --target aab -- -j"$(nproc)"

echo ""
found=0
for artifact in \
    "$BUILD_DIR/android-build/build/outputs/apk/debug/android-build-debug.apk" \
    "$BUILD_DIR/android-build/build/outputs/apk/release/android-build-release.apk" \
    "$BUILD_DIR/android-build/${APP_TARGET}.apk" \
    "$BUILD_DIR/android-build/build/outputs/bundle/debug/android-build-debug.aab" \
    "$BUILD_DIR/android-build/build/outputs/bundle/release/android-build-release.aab"
do
    if [[ -f "$artifact" ]]; then
        echo "  $artifact"
        found=1
    fi
done

if [[ "$found" -eq 0 ]]; then
    echo "WARNING: no APK/AAB found; check $BUILD_DIR/android-build for Gradle output."
    echo "  cd $BUILD_DIR/android-build && ./gradlew bundleRelease --stacktrace"
fi

# Native debug symbols: AGP embeds them in the AAB when debugSymbolLevel=FULL.
# Confirm locally here; Play extracts them automatically on upload.
if [[ "$BUILD_TYPE" == "release" ]]; then
    AAB_RELEASE="$BUILD_DIR/android-build/build/outputs/bundle/release/android-build-release.aab"
    SYMBOLS_ZIP="$BUILD_DIR/android-build/build/outputs/native-debug-symbols/release/native-debug-symbols.zip"
    echo ""
    echo "============================================================"
    echo "  Native debug symbols"
    echo "============================================================"
    if [[ -f "$AAB_RELEASE" ]]; then
        # grep -q closes the pipe early (SIGPIPE); with pipefail that looks like
        # a miss even when the AAB does contain debugsymbols.
        symbols=$(unzip -l "$AAB_RELEASE" | grep "com.android.tools.build.debugsymbols" || true)
        if [[ -n "$symbols" ]]; then
            echo "  Bundled in AAB (Play extracts these automatically):"
            echo "$symbols" | sed 's/^/    /'
        else
            echo "  WARNING: $AAB_RELEASE has no BUNDLE-METADATA/com.android.tools.build.debugsymbols."
            echo "  AGP could not extract DWARF from the packaged .so files."
            echo "  Upload a ZIP of unstripped ABI folders only if Play Console still asks."
        fi
    fi
    if [[ -f "$SYMBOLS_ZIP" ]]; then
        echo "  $SYMBOLS_ZIP"
    fi
fi

# 16 KB page-size compatibility (Play requirement for 64-bit apps on API 35+).
echo ""
echo "============================================================"
echo "  16 KB page-size compatibility"
echo "============================================================"
APK_CHECK=""
AAB_CHECK=""
for artifact in \
    "$BUILD_DIR/android-build/build/outputs/apk/${BUILD_TYPE}/android-build-${BUILD_TYPE}.apk" \
    "$BUILD_DIR/android-build/build/outputs/apk/${BUILD_TYPE}/android-build-${BUILD_TYPE}-unsigned.apk" \
    "$BUILD_DIR/android-build/${APP_TARGET}.apk"
do
    if [[ -f "$artifact" ]]; then
        APK_CHECK="$artifact"
        break
    fi
done
AAB_CANDIDATE="$BUILD_DIR/android-build/build/outputs/bundle/${BUILD_TYPE}/android-build-${BUILD_TYPE}.aab"
if [[ -f "$AAB_CANDIDATE" ]]; then
    AAB_CHECK="$AAB_CANDIDATE"
fi
if [[ -z "$APK_CHECK" ]]; then
    echo "WARNING: no APK found to check for 16 KB alignment."
else
    "$SCRIPT_DIR/check_16kb_alignment.sh" "$APK_CHECK" ${AAB_CHECK:+"$AAB_CHECK"}
fi
