#!/usr/bin/env bash
# =============================================================================
# run_emulator.sh — Start Android emulator, install the built APK, and run it.
#
# Usage:
#   ./scripts/run_emulator.sh [avd_name]
#   ./scripts/run_emulator.sh --tablet 7     # Play 7-inch tablet AVD (1200×1920)
#   ./scripts/run_emulator.sh --tablet 10    # Play 10-inch tablet AVD (1600×2560)
#   ./scripts/run_emulator.sh --gesture      # Pixel 7a with gesture navigation
#   ./scripts/run_emulator.sh --screenshot [name]
#   ./scripts/run_emulator.sh --logcat
# =============================================================================

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./scripts/run_emulator.sh [avd_name]
  ./scripts/run_emulator.sh --tablet 7|10
  ./scripts/run_emulator.sh --gesture
  ./scripts/run_emulator.sh --screenshot [name]
  ./scripts/run_emulator.sh --logcat
  ./scripts/run_emulator.sh --help

Tablet AVDs match Google Play listing slots (7-inch and 10-inch). They are
created on first use from the local x86_64 Google APIs system image:

  PlayTablet7_x86_64   Nexus 7 (2013), portrait 1200×1920
  PlayTablet10_x86_64  Nexus 10,       portrait 1600×2560

--gesture creates and boots Pixel7a_gesture_x86_64 (Pixel 7a, 1080×2400)
and switches the in-display navbar to gesture / edge-to-edge mode. Use this
to reproduce Pixel-style SafeArea overlap. Pixel7a_x86_64 stays on
3-button navigation for Play phone screenshots.

The app is portrait-locked; tablet screenshots are captured in portrait.

After a device is running, navigate to a screen and capture a PNG under
packaging/google-play/screenshots/{phone,tablet7,tablet10}/:

  ./scripts/run_emulator.sh --screenshot home
  ./scripts/run_emulator.sh --screenshot history
  ./scripts/run_emulator.sh --screenshot session
  ./scripts/run_emulator.sh --screenshot settings
  ./scripts/run_emulator.sh --screenshot guide
  ./scripts/run_emulator.sh --screenshot privacy

Known names write numbered files (1_home.png … 6_privacy.png).
EOF
}

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}"
EMULATOR_CMD="$ANDROID_SDK/emulator/emulator"
ADB_CMD="$ANDROID_SDK/platform-tools/adb"
AVDMANAGER_CMD="$ANDROID_SDK/cmdline-tools/latest/bin/avdmanager"

ANDROID_PACKAGE="${ANDROID_PACKAGE:-by.intoncore.SpeechRateMeter2}"
BOOT_TIMEOUT_SEC="${BOOT_TIMEOUT_SEC:-120}"

TABLET7_AVD="${TABLET7_AVD:-PlayTablet7_x86_64}"
TABLET10_AVD="${TABLET10_AVD:-PlayTablet10_x86_64}"
TABLET7_DEVICE="Nexus 7 2013"
TABLET10_DEVICE="Nexus 10"
TABLET7_WM_SIZE="1200x1920"
TABLET10_WM_SIZE="1600x2560"

GESTURE_AVD="${GESTURE_AVD:-Pixel7a_gesture_x86_64}"
GESTURE_DEVICE="pixel_7a"
GESTURE_WM_SIZE="1080x2400"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SCREENSHOT_ROOT="$PROJECT_ROOT/packaging/google-play/screenshots"
APP_TARGET="appspeech-rate-meter-2"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
list_avds() {
    "$EMULATOR_CMD" -list-avds 2>/dev/null || true
}

avd_exists() {
    local name="$1"
    list_avds | grep -Fxq "$name"
}

online_serials() {
    "$ADB_CMD" devices | awk 'NR>1 && $2=="device" {print $1}'
}

emulator_serials() {
    "$ADB_CMD" devices | awk '/^emulator-/ {print $1}'
}

online_emulator_serial() {
    "$ADB_CMD" devices | awk '/^emulator-/ && $2=="device" {print $1; exit}'
}

pin_adb_serial() {
    local serial
    local serials=()
    serial="$(online_emulator_serial)"
    if [[ -z "$serial" ]]; then
        mapfile -t serials < <(online_serials)
        serial="${serials[0]:-}"
    fi
    if [[ -n "$serial" ]]; then
        export ANDROID_SERIAL="$serial"
        echo "Using adb serial: $ANDROID_SERIAL"
    fi
}

running_avd_name() {
    local serial="${1:-}"
    local args=()
    if [[ -n "$serial" ]]; then
        args=(-s "$serial")
    fi
    timeout 5 "$ADB_CMD" "${args[@]}" emu avd name 2>/dev/null \
        | awk 'NF && $1 != "OK" {print $1; exit}' || true
}

pick_sys_image() {
    local pkg dir
    for pkg in \
        "system-images;android-34;google_apis;x86_64" \
        "system-images;android-36;google_apis;x86_64" \
        "system-images;android-36;google_apis_playstore;x86_64"; do
        dir="$ANDROID_SDK/${pkg//;//}"
        if [[ -d "$dir" ]]; then
            echo "$pkg"
            return 0
        fi
    done
    return 1
}

patch_avd_config() {
    local cfg="$1"
    local wm_size="$2"
    local width height
    width="${wm_size%x*}"
    height="${wm_size#*x}"
    [[ -f "$cfg" ]] || return 0

    # Keyboard helps while posing screenshots; portrait matches the app lock.
    if grep -q '^hw.keyboard=' "$cfg"; then
        sed -i 's/^hw.keyboard=.*/hw.keyboard=yes/' "$cfg"
    else
        echo "hw.keyboard=yes" >> "$cfg"
    fi
    if grep -q '^hw.initialOrientation=' "$cfg"; then
        sed -i 's/^hw.initialOrientation=.*/hw.initialOrientation=portrait/' "$cfg"
    else
        echo "hw.initialOrientation=portrait" >> "$cfg"
    fi
    if grep -q '^hw.lcd.width=' "$cfg"; then
        sed -i "s/^hw.lcd.width=.*/hw.lcd.width=${width}/" "$cfg"
    else
        echo "hw.lcd.width=${width}" >> "$cfg"
    fi
    if grep -q '^hw.lcd.height=' "$cfg"; then
        sed -i "s/^hw.lcd.height=.*/hw.lcd.height=${height}/" "$cfg"
    else
        echo "hw.lcd.height=${height}" >> "$cfg"
    fi
}

ensure_avd() {
    local name="$1"
    local device="$2"
    local wm_size="$3"
    local pkg cfg

    if avd_exists "$name"; then
        echo "Using existing AVD: $name"
        return 0
    fi

    if [[ ! -x "$AVDMANAGER_CMD" ]]; then
        echo "ERROR: avdmanager not found at $AVDMANAGER_CMD"
        echo "Install Android SDK cmdline-tools, or create '$name' in Android Studio."
        exit 1
    fi

    if [[ -x "$JAVA_HOME/bin/java" ]]; then
        export JAVA_HOME
        export PATH="$JAVA_HOME/bin:$PATH"
    fi

    pkg="$(pick_sys_image || true)"
    if [[ -z "$pkg" ]]; then
        echo "ERROR: No x86_64 Android system image found under $ANDROID_SDK/system-images."
        echo "Install one, for example:"
        echo "  sdkmanager \"system-images;android-34;google_apis;x86_64\""
        exit 1
    fi

    echo "Creating AVD '$name' ($device, $wm_size) from $pkg ..."
    local create_log
    create_log="$(mktemp)"
    # "no" answers the optional custom hardware-profile prompt.
    # avdmanager always warns about missing per-image devices.xml files.
    if ! printf 'no\n' | "$AVDMANAGER_CMD" create avd \
        --name "$name" \
        --package "$pkg" \
        --device "$device" \
        --sdcard 512M \
        --force >"$create_log" 2>&1; then
        grep -v 'Could not load devices' "$create_log" || cat "$create_log"
        rm -f "$create_log"
        echo "ERROR: Failed to create AVD '$name'."
        exit 1
    fi
    grep -v 'Could not load devices' "$create_log" || true
    rm -f "$create_log"

    cfg="$HOME/.android/avd/${name}.avd/config.ini"
    patch_avd_config "$cfg" "$wm_size"

    if ! avd_exists "$name"; then
        echo "ERROR: Failed to create AVD '$name'."
        exit 1
    fi
    echo "Created AVD: $name"
}

ensure_tablet_avd() {
    ensure_avd "$1" "$2" "$3"
}

# Gesture vs 3-button is a SystemUI overlay, not an AVD hardware property.
# Pixel phones ship with gesture nav; Play screenshot AVDs stay on 3-button.
enable_gesture_navigation() {
    echo "Enabling gesture navigation (edge-to-edge system bars)..."
    "$ADB_CMD" shell cmd overlay enable-exclusive --category android.internal.systemui.navbar \
        com.android.internal.systemui.navbar.gestural >/dev/null 2>&1 || \
    "$ADB_CMD" shell cmd overlay enable-exclusive \
        com.android.internal.systemui.navbar.gestural >/dev/null 2>&1 || true
    "$ADB_CMD" shell cmd overlay disable com.android.internal.systemui.navbar.threebutton >/dev/null 2>&1 || true
    "$ADB_CMD" shell cmd overlay disable com.android.internal.systemui.navbar.twobutton >/dev/null 2>&1 || true
    "$ADB_CMD" shell cmd overlay enable com.android.internal.systemui.navbar.gestural >/dev/null 2>&1 || true
    "$ADB_CMD" shell settings put secure navigation_mode 2 >/dev/null 2>&1 || true

    local mode
    mode=$("$ADB_CMD" shell settings get secure navigation_mode 2>/dev/null || echo "?")
    mode="${mode//[$'\r\n']/}"
    echo "navigation_mode=${mode} (2 = gesture, 0 = 3-button)"
}

uses_gesture_nav() {
    local name="${1:-}"
    [[ "${WANT_GESTURE:-0}" -eq 1 ]] && return 0
    [[ "$name" == "$GESTURE_AVD" ]] && return 0
    [[ "$name" == *_gesture* ]] && return 0
    return 1
}

kill_emulators() {
    local serial
    local any=0
    while read -r serial; do
        [[ -z "$serial" ]] && continue
        echo "Stopping $serial ..."
        "$ADB_CMD" -s "$serial" emu kill >/dev/null 2>&1 || true
        any=1
    done < <(emulator_serials)

    if [[ "$any" -eq 0 ]]; then
        return 0
    fi

    local waited=0
    while [[ $waited -lt 30 ]]; do
        if [[ -z "$(emulator_serials)" ]]; then
            sleep 2
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    echo "WARNING: An emulator serial is still listed after emu kill; continuing."
}

wait_for_boot() {
    local emulator_pid="$1"
    local emulator_log="$2"
    local timeout_sec="$3"
    local elapsed=5

    echo "Waiting for emulator to boot (timeout: ${timeout_sec}s)..."
    while true; do
        if ! kill -0 "$emulator_pid" 2>/dev/null; then
            echo ""
            echo "ERROR: Emulator process died unexpectedly."
            echo "Last 20 lines of emulator log ($emulator_log):"
            tail -20 "$emulator_log" 2>/dev/null || true
            exit 1
        fi
        local boot_status
        boot_status=$(timeout 5 "$ADB_CMD" shell getprop sys.boot_completed 2>/dev/null || echo "0")
        boot_status="${boot_status//[$'\r\n']/}"
        if [[ "$boot_status" == "1" ]]; then
            break
        fi
        if [[ $elapsed -ge $timeout_sec ]]; then
            echo ""
            echo "ERROR: Emulator did not boot within ${timeout_sec} seconds."
            echo "Check emulator log: $emulator_log"
            kill "$emulator_pid" 2>/dev/null || true
            exit 1
        fi
        sleep 2
        elapsed=$((elapsed + 2))
        echo -n "."
    done
    echo ""
    echo "Emulator is online!"
}

apply_tablet_display() {
    local wm_size="$1"
    # Deterministic Play screenshot resolution; app is portrait-only.
    "$ADB_CMD" shell settings put system accelerometer_rotation 0 >/dev/null 2>&1 || true
    "$ADB_CMD" shell wm size "$wm_size" >/dev/null 2>&1 || true
    echo "Display size set to $wm_size (portrait)"
}

play_slot_from_avd() {
    local avd="$1"
    case "$avd" in
        "$TABLET7_AVD"|*Tablet7*|*[Tt]ablet*7*) echo "tablet7" ;;
        "$TABLET10_AVD"|*Tablet10*|*[Tt]ablet*10*) echo "tablet10" ;;
        *) echo "" ;;
    esac
}

play_slot_from_wm() {
    local raw width height short long
    raw=$("$ADB_CMD" shell wm size 2>/dev/null | awk -F': ' '/Physical size|Override size/{print $2}' | tail -1)
    raw="${raw//[$'\r\n']/}"
    width="${raw%x*}"
    height="${raw#*x}"
    if [[ -z "$width" || -z "$height" || "$width" == "$raw" ]]; then
        echo "phone"
        return
    fi
    if (( width < height )); then
        short="$width"
        long="$height"
    else
        short="$height"
        long="$width"
    fi
    # 7-inch Play shots are ~1200×1920; 10-inch are ~1600×2560.
    # Phones like Pixel 7a (1080×2400) must not land in a tablet slot.
    if (( short >= 1500 && long >= 2200 )); then
        echo "tablet10"
    elif (( short >= 1100 && long >= 1700 && long <= 2100 )); then
        echo "tablet7"
    else
        echo "phone"
    fi
}

detect_play_slot() {
    local avd slot
    avd="$(running_avd_name)"
    slot="$(play_slot_from_avd "$avd")"
    if [[ -n "$slot" ]]; then
        echo "$slot"
        return
    fi
    play_slot_from_wm
}

# Map --screenshot names to Play listing filenames (1_home.png …).
screenshot_basename() {
    local name="$1"
    case "$name" in
        home|1_home) echo "1_home" ;;
        history|2_history) echo "2_history" ;;
        session|charts|3_session) echo "3_session" ;;
        settings|4_settings) echo "4_settings" ;;
        guide|user_guide|5_user_guide) echo "5_user_guide" ;;
        privacy|6_privacy) echo "6_privacy" ;;
        *) echo "$name" ;;
    esac
}

capture_screenshot() {
    local name="$1"
    local slot dest tmp
    local serials
    serials="$(online_serials || true)"
    if [[ -z "$serials" ]]; then
        echo "ERROR: No device online. Start an emulator first."
        exit 1
    fi

    name="$(screenshot_basename "$name")"
    slot="$(detect_play_slot)"
    mkdir -p "$SCREENSHOT_ROOT/$slot"
    dest="$SCREENSHOT_ROOT/$slot/${name}.png"
    tmp="$(mktemp --suffix=.png)"

    echo "Capturing $slot screenshot: $name"
    "$ADB_CMD" exec-out screencap -p > "$tmp"
    if [[ ! -s "$tmp" ]]; then
        rm -f "$tmp"
        echo "ERROR: screencap produced an empty file."
        exit 1
    fi

    # Play Console wants JPEG or 24-bit PNG with no alpha.
    if command -v convert >/dev/null 2>&1; then
        convert "$tmp" -alpha remove -alpha off "PNG24:$dest"
        rm -f "$tmp"
    else
        mv "$tmp" "$dest"
        echo "WARNING: ImageMagick 'convert' not found; PNG alpha was not stripped."
    fi

    local dims=""
    if command -v identify >/dev/null 2>&1; then
        dims=" ($(identify -format '%wx%h' "$dest"))"
    fi
    echo "Saved $dest${dims}"
    echo "Upload this file to the Play Console ${slot} screenshot slot."
}

find_apk() {
    local build_dir path
    for build_dir in \
        "$PROJECT_ROOT/build_android_x86_64" \
        "$PROJECT_ROOT/build_android_arm64-v8a"; do
        for path in \
            "$build_dir/android-build/build/outputs/apk/debug/android-build-debug.apk" \
            "$build_dir/android-build/build/outputs/apk/release/android-build-release.apk" \
            "$build_dir/android-build/${APP_TARGET}.apk" \
            "$build_dir/android-build/build/outputs/apk/release/android-build-release-unsigned.apk"; do
            if [[ -f "$path" ]]; then
                echo "$path"
                return 0
            fi
        done
    done
    return 1
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
SELECTED_AVD=""
TABLET_SIZE=""
TABLET_WM_SIZE=""
DO_SCREENSHOT=0
SCREENSHOT_NAME=""
LOGCAT_ONLY=0
WANT_GESTURE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --logcat)
            LOGCAT_ONLY=1
            shift
            ;;
        --gesture)
            WANT_GESTURE=1
            shift
            ;;
        --tablet)
            if [[ "${2:-}" != "7" && "${2:-}" != "10" ]]; then
                echo "ERROR: --tablet requires 7 or 10 (Play Console tablet slots)."
                usage
                exit 1
            fi
            TABLET_SIZE="$2"
            shift 2
            ;;
        --tablet7)
            TABLET_SIZE="7"
            shift
            ;;
        --tablet10)
            TABLET_SIZE="10"
            shift
            ;;
        --screenshot)
            DO_SCREENSHOT=1
            if [[ $# -ge 2 && "$2" != -* ]]; then
                SCREENSHOT_NAME="$2"
                shift 2
            else
                shift
            fi
            ;;
        --)
            shift
            break
            ;;
        -*)
            echo "ERROR: unknown option '$1'"
            usage
            exit 1
            ;;
        *)
            if [[ -n "$SELECTED_AVD" ]]; then
                echo "ERROR: unexpected extra argument '$1'"
                usage
                exit 1
            fi
            SELECTED_AVD="$1"
            shift
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Pre-flight
# ---------------------------------------------------------------------------
for cmd in "$EMULATOR_CMD" "$ADB_CMD"; do
    if [[ ! -x "$cmd" ]]; then
        echo "ERROR: '$cmd' not found or not executable."
        echo "Set ANDROID_SDK or install Android SDK tools."
        exit 1
    fi
done

if [[ "$LOGCAT_ONLY" -eq 1 ]]; then
    LOGCAT_LOG="$PROJECT_ROOT/build_android_arm64-v8a/logcat.log"
    echo "Clearing and capturing logcat to: $LOGCAT_LOG"
    "$ADB_CMD" logcat -c 2>/dev/null || true
    "$ADB_CMD" logcat -v time | tee "$LOGCAT_LOG"
    exit 0
fi

# Screenshot-only against whatever is already running.
if [[ "$DO_SCREENSHOT" -eq 1 && -z "$TABLET_SIZE" && -z "$SELECTED_AVD" && "$WANT_GESTURE" -eq 0 ]]; then
    if [[ -z "$SCREENSHOT_NAME" ]]; then
        SCREENSHOT_NAME="shot-$(date +%Y%m%d-%H%M%S)"
    fi
    capture_screenshot "$SCREENSHOT_NAME"
    exit 0
fi

if [[ -n "$TABLET_SIZE" && -n "$SELECTED_AVD" ]]; then
    echo "ERROR: pass either --tablet 7|10 or an AVD name, not both."
    exit 1
fi
if [[ -n "$TABLET_SIZE" && "$WANT_GESTURE" -eq 1 ]]; then
    echo "ERROR: pass either --tablet or --gesture, not both."
    exit 1
fi
if [[ "$WANT_GESTURE" -eq 1 && -n "$SELECTED_AVD" && "$SELECTED_AVD" != "$GESTURE_AVD" ]]; then
    echo "ERROR: --gesture uses $GESTURE_AVD; do not pass a different AVD name."
    exit 1
fi

if [[ "$TABLET_SIZE" == "7" ]]; then
    SELECTED_AVD="$TABLET7_AVD"
    TABLET_WM_SIZE="$TABLET7_WM_SIZE"
    ensure_tablet_avd "$TABLET7_AVD" "$TABLET7_DEVICE" "$TABLET7_WM_SIZE"
elif [[ "$TABLET_SIZE" == "10" ]]; then
    SELECTED_AVD="$TABLET10_AVD"
    TABLET_WM_SIZE="$TABLET10_WM_SIZE"
    ensure_tablet_avd "$TABLET10_AVD" "$TABLET10_DEVICE" "$TABLET10_WM_SIZE"
fi

if [[ "$WANT_GESTURE" -eq 1 ]]; then
    SELECTED_AVD="$GESTURE_AVD"
fi
if uses_gesture_nav "$SELECTED_AVD"; then
    ensure_avd "$GESTURE_AVD" "$GESTURE_DEVICE" "$GESTURE_WM_SIZE"
    SELECTED_AVD="$GESTURE_AVD"
fi

if [[ ( -n "$TABLET_SIZE" || "$WANT_GESTURE" -eq 1 ) && "${BOOT_TIMEOUT_SEC}" -lt 180 ]]; then
    # First boot of a new tablet/gesture AVD is slower than a phone snapshot.
    BOOT_TIMEOUT_SEC=240
fi

APK_PATH="$(find_apk || true)"
if [[ -z "$APK_PATH" ]]; then
    echo "ERROR: No built APK found."
    echo "Please build the project first: ./scripts/build_android.sh x86_64 debug"
    exit 1
fi

echo "Found APK: $APK_PATH"
BUILD_DIR="$PROJECT_ROOT/$(echo "$APK_PATH" | sed -n 's|.*/\(build_android_[^/]*\)/.*|\1|p')"
if [[ -z "$BUILD_DIR" || "$BUILD_DIR" == "$PROJECT_ROOT/" ]]; then
    BUILD_DIR="$PROJECT_ROOT/build_android_x86_64"
fi

# ---------------------------------------------------------------------------
# 1. Start Emulator
# ---------------------------------------------------------------------------
CURRENT_AVD="$(running_avd_name)"
RUNNING_DEVICES="$(online_serials || true)"

need_start=1
if [[ -n "$RUNNING_DEVICES" ]]; then
    if [[ -z "$SELECTED_AVD" ]]; then
        echo "Using already running emulator/device..."
        need_start=0
    elif [[ -n "$CURRENT_AVD" && "$CURRENT_AVD" == "$SELECTED_AVD" ]]; then
        echo "Requested AVD '$SELECTED_AVD' is already running."
        need_start=0
    else
        echo "A different device is running${CURRENT_AVD:+ ($CURRENT_AVD)}; replacing it with '$SELECTED_AVD'."
        kill_emulators
    fi
fi

if [[ "$need_start" -eq 1 ]]; then
    echo "No matching emulator detected. Starting one..."

    readarray -t AVDS < <(list_avds)
    if [[ ${#AVDS[@]} -eq 0 ]]; then
        echo "ERROR: No Android Virtual Devices (AVDs) found."
        echo "Create one in Android Studio, or use: ./scripts/run_emulator.sh --gesture"
        exit 1
    fi

    if [[ -z "$SELECTED_AVD" ]]; then
        SELECTED_AVD="${AVDS[0]}"
    elif ! avd_exists "$SELECTED_AVD"; then
        echo "ERROR: AVD '$SELECTED_AVD' not found."
        echo "Available AVDs:"
        printf '  %s\n' "${AVDS[@]}"
        exit 1
    fi

    echo "Starting emulator with AVD: $SELECTED_AVD..."
    EMULATOR_LOG="$BUILD_DIR/emulator.log"
    mkdir -p "$BUILD_DIR"
    "$EMULATOR_CMD" -avd "$SELECTED_AVD" -no-snapshot-load > "$EMULATOR_LOG" 2>&1 &
    EMULATOR_PID=$!

    echo "Waiting for emulator to initialize..."
    sleep 5
    wait_for_boot "$EMULATOR_PID" "$EMULATOR_LOG" "$BOOT_TIMEOUT_SEC"
fi

pin_adb_serial

if [[ -n "$TABLET_WM_SIZE" ]]; then
    apply_tablet_display "$TABLET_WM_SIZE"
fi

if uses_gesture_nav "${SELECTED_AVD:-}" || uses_gesture_nav "$(running_avd_name)"; then
    enable_gesture_navigation
fi

# ---------------------------------------------------------------------------
# 2. Install and Run the APK
# ---------------------------------------------------------------------------
echo "Installing APK..."
"$ADB_CMD" install -r "$APK_PATH"

echo "Launching application..."
LOGCAT_LOG="$BUILD_DIR/logcat.log"
"$ADB_CMD" logcat -c 2>/dev/null || true
"$ADB_CMD" logcat -v time > "$LOGCAT_LOG" 2>&1 &
LOGCAT_PID=$!

"$ADB_CMD" shell am start -n "$ANDROID_PACKAGE/org.qtproject.qt.android.bindings.QtActivity"

echo "Waiting for app startup..."
sleep 5

APP_PID=$("$ADB_CMD" shell pidof "$ANDROID_PACKAGE" 2>/dev/null || echo "")
APP_PID="${APP_PID//[$'\r\n']/}"
if [[ -z "$APP_PID" ]]; then
    echo ""
    echo "WARNING: App process not found — it may have crashed on startup."
    echo "--- Last 40 lines of logcat (filtered for errors & Qt) ---"
    grep -iE 'FATAL|ERROR|Qt|speech|SpeechRate|crash|signal|SIGSEGV|SIGABRT|dlopen|UnsatisfiedLinkError|NoClassDefFoundError' "$LOGCAT_LOG" | tail -40 || true
    echo "--- Full logcat saved to: $LOGCAT_LOG ---"
else
    echo "App is running (PID: $APP_PID)"
fi

echo "Logcat capturing to: $LOGCAT_LOG (PID: $LOGCAT_PID)"
echo "To stop logcat: kill $LOGCAT_PID"

if [[ -n "$TABLET_SIZE" ]]; then
    echo ""
    echo "Tablet $TABLET_SIZE-inch is ready for Play screenshots ($TABLET_WM_SIZE)."
    echo "Navigate to a screen, then capture:"
    echo "  ./scripts/run_emulator.sh --screenshot home"
    echo "  ./scripts/run_emulator.sh --screenshot history"
    echo "  ./scripts/run_emulator.sh --screenshot session"
    echo "  ./scripts/run_emulator.sh --screenshot settings"
    echo "  ./scripts/run_emulator.sh --screenshot guide"
    echo "  ./scripts/run_emulator.sh --screenshot privacy"
    echo "Files go to packaging/google-play/screenshots/tablet${TABLET_SIZE}/"
fi

if uses_gesture_nav "${SELECTED_AVD:-}"; then
    echo ""
    echo "Gesture-nav AVD is ready ($GESTURE_AVD, $GESTURE_WM_SIZE)."
    echo "Confirm a home-gesture pill at the bottom, not 3-button Back/Home/Recents."
    echo "This is the SafeArea / edge-to-edge test device."
fi

if [[ "$DO_SCREENSHOT" -eq 1 ]]; then
    if [[ -z "$SCREENSHOT_NAME" ]]; then
        SCREENSHOT_NAME="shot-$(date +%Y%m%d-%H%M%S)"
    fi
    echo ""
    if [[ -t 0 ]]; then
        echo "Press Enter when the screen is ready to capture '$SCREENSHOT_NAME'..."
        read -r || true
    else
        echo "Waiting 8s before capturing '$SCREENSHOT_NAME'..."
        sleep 8
    fi
    capture_screenshot "$SCREENSHOT_NAME"
fi
