#!/usr/bin/env bash
# =============================================================================
# check_16kb_alignment.sh — Verify Play's 16 KB page-size packaging.
#
# Checks:
#   1. ELF LOAD segment alignment of every arm64-v8a / x86_64 .so (>= 2**14)
#   2. APK zip alignment of uncompressed native libs (zipalign -P 16)
#   3. AAB BundleConfig native-library alignment (PAGE_ALIGNMENT_16K)
#
# Usage:
#   ./scripts/check_16kb_alignment.sh <apk> [aab]
#   ./scripts/check_16kb_alignment.sh --dir <extracted-lib-dir>
#
# Exit 0 if all 64-bit libraries are ALIGNED and zip/AAB checks pass.
# armeabi-v7a is reported but does not fail the check (32-bit is exempt).
# =============================================================================

set -euo pipefail

ANDROID_SDK="${ANDROID_SDK:-$HOME/Android/Sdk}"
ANDROID_NDK="${ANDROID_NDK:-$ANDROID_SDK/ndk/27.2.12479018}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RED=$'\033[31m'
GREEN=$'\033[32m'
YELLOW=$'\033[33m'
RESET=$'\033[0m'

usage() {
    echo "Usage: $0 <apk> [aab]"
    echo "       $0 --dir <directory-of-.so-files>"
    exit 2
}

find_tool() {
    local name="$1"
    shift
    local candidate
    for candidate in "$@"; do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
            echo "$candidate"
            return 0
        fi
    done
    if command -v "$name" >/dev/null 2>&1; then
        command -v "$name"
        return 0
    fi
    return 1
}

PREBUILT="$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/* | awk '{print $1}')"
OBJDUMP="$(find_tool llvm-objdump \
    "$PREBUILT/bin/llvm-objdump" \
    "$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump" \
    || true)"
ZIPALIGN="$(find_tool zipalign \
    "$ANDROID_SDK/build-tools/36.0.0/zipalign" \
    "$ANDROID_SDK/build-tools/35.0.0/zipalign" \
    || true)"
if [[ -z "$ZIPALIGN" ]]; then
    ZIPALIGN="$(ls -1 "$ANDROID_SDK"/build-tools/*/zipalign 2>/dev/null | sort -V | tail -1 || true)"
fi

if [[ -z "$OBJDUMP" ]]; then
    echo "ERROR: llvm-objdump not found. Set ANDROID_NDK."
    exit 1
fi

# ALIGNED when every LOAD align is 2**14 (16 KB) or higher.
is_aligned() {
    local so="$1"
    local aligns
    aligns="$("$OBJDUMP" -p "$so" 2>/dev/null | awk '/LOAD/ { print $NF }')"
    if [[ -z "$aligns" ]]; then
        return 1
    fi
    local a
    for a in $aligns; do
        if [[ ! "$a" =~ ^2\*\*(1[4-9]|[2-9][0-9]|[1-9][0-9]{2,})$ ]]; then
            return 1
        fi
    done
    return 0
}

first_align() {
    "$OBJDUMP" -p "$1" 2>/dev/null | awk '/LOAD/ { print $NF; exit }'
}

needs_16kb() {
    # 32-bit ABIs are exempt. Anything else (including a bare .so path) is checked.
    case "$1" in
        *armeabi-v7a*|*armeabi*|*armv7*|*x86/*|*/i386/*|*/arm/*) return 1 ;;
        *) return 0 ;;
    esac
}

UNALIGNED=()
ALIGNED_COUNT=0
SKIPPED_32BIT=0

check_so() {
    local so="$1"
    local label="${2:-$so}"
    local align
    align="$(first_align "$so")"
    if needs_16kb "$label" || needs_16kb "$so"; then
        if is_aligned "$so"; then
            if [[ "${VERBOSE:-0}" != "0" ]]; then
                echo "  ${GREEN}ALIGNED${RESET}    ($align)  $label"
            fi
            ALIGNED_COUNT=$((ALIGNED_COUNT + 1))
        else
            echo "  ${RED}UNALIGNED${RESET}  (${align:-unknown})  $label"
            UNALIGNED+=("$label")
        fi
    else
        if [[ "${VERBOSE:-0}" != "0" ]]; then
            echo "  skip 32-bit ($align)  $label"
        fi
        SKIPPED_32BIT=$((SKIPPED_32BIT + 1))
    fi
}

extract_and_check_zip() {
    local archive="$1"
    local pattern="$2"
    local tmp
    tmp="$(mktemp -d)"
    # shellcheck disable=SC2064
    trap "rm -rf '$tmp'" RETURN
    unzip -q -o "$archive" "$pattern" -d "$tmp" 2>/dev/null || true
    local so
    local found=0
    while IFS= read -r -d '' so; do
        found=1
        check_so "$so" "${so#"$tmp"/}"
    done < <(find "$tmp" -type f -name '*.so' -print0 | sort -z)
    if [[ "$found" -eq 0 ]]; then
        echo "ERROR: no .so files matching $pattern in $archive"
        return 1
    fi
}

parse_aab_native_alignment() {
    local aab="$1"
    python3 - "$aab" <<'PY'
import sys
from zipfile import ZipFile

ALIGN = {0: "UNSPECIFIED", 1: "PAGE_ALIGNMENT_4K", 2: "PAGE_ALIGNMENT_16K"}

def decode_varint(b, i):
    n = 0
    s = 0
    while True:
        if i >= len(b):
            raise ValueError("truncated varint")
        x = b[i]
        i += 1
        n |= (x & 0x7F) << s
        if x < 0x80:
            return n, i
        s += 7

def fields(b):
    i = 0
    out = []
    while i < len(b):
        key, i = decode_varint(b, i)
        fn, wt = key >> 3, key & 7
        if wt == 0:
            v, i = decode_varint(b, i)
            out.append((fn, "varint", v))
        elif wt == 1:
            out.append((fn, "i64", b[i:i+8]))
            i += 8
        elif wt == 2:
            ln, i = decode_varint(b, i)
            out.append((fn, "bytes", b[i:i+ln]))
            i += ln
        elif wt == 5:
            out.append((fn, "i32", b[i:i+4]))
            i += 4
        else:
            break
    return out

aab = sys.argv[1]
data = ZipFile(aab).read("BundleConfig.pb")
cfg = {fn: (wt, val) for fn, wt, val in fields(data)}
opt = cfg.get(2)
if not opt or opt[0] != "bytes":
    print("MISSING_OPTIMIZATIONS")
    sys.exit(0)
uncompress = None
for fn, wt, val in fields(opt[1]):
    if fn == 2 and wt == "bytes":
        uncompress = {f: v for f, _, v in fields(val)}
if uncompress is None:
    print("MISSING_UNCOMPRESS_NATIVE_LIBRARIES")
    sys.exit(0)
enabled = int(uncompress.get(1, 0) or 0)
alignment = int(uncompress.get(2, 0) or 0)
print(f"enabled={enabled}")
print(f"alignment={ALIGN.get(alignment, str(alignment))}")
sys.exit(0)
PY
}

FAILED=0

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" || $# -lt 1 ]]; then
    usage
fi

if [[ "${1:-}" == "--dir" ]]; then
    DIR="${2:-}"
    [[ -d "$DIR" ]] || usage
    echo "=== ELF alignment ($DIR) ==="
    while IFS= read -r -d '' so; do
        check_so "$so"
    done < <(find "$DIR" -type f -name '*.so' -print0 | sort -z)
else
    APK="$1"
    AAB="${2:-}"
    [[ -f "$APK" ]] || { echo "ERROR: APK not found: $APK"; exit 1; }

    echo "=== ELF alignment ($APK) ==="
    extract_and_check_zip "$APK" "lib/*/*.so"

    echo
    echo "=== APK zip alignment (16 KB pages) ==="
    if [[ -z "$ZIPALIGN" || ! -x "$ZIPALIGN" ]]; then
        echo "ERROR: zipalign not found (need build-tools 35+ with -P)."
        FAILED=1
    else
        zipalign_help="$("$ZIPALIGN" 2>&1 || true)"
        if ! echo "$zipalign_help" | grep -q -- '-P <pagesize_kb>'; then
            echo "ERROR: $ZIPALIGN does not support -P 16. Install build-tools 35.0.0 or newer."
            FAILED=1
        else
            zip_out="$("$ZIPALIGN" -c -P 16 -v 4 "$APK" 2>&1 || true)"
            echo "$zip_out" | grep -E 'lib/(arm64-v8a|x86_64)|Verification|FAILED' || true
            if echo "$zip_out" | grep -q 'Verification successful'; then
                echo "  ${GREEN}zipalign: Verification successful${RESET}"
            else
                echo "  ${RED}zipalign: Verification failed${RESET}"
                FAILED=1
            fi
        fi
    fi

    if [[ -n "$AAB" ]]; then
        [[ -f "$AAB" ]] || { echo "ERROR: AAB not found: $AAB"; exit 1; }
        echo
        echo "=== ELF alignment ($AAB) ==="
        extract_and_check_zip "$AAB" "base/lib/*/*.so"
        echo
        echo "=== AAB BundleConfig native alignment ==="
        cfg="$(parse_aab_native_alignment "$AAB")"
        echo "$cfg" | sed 's/^/  /'
        if echo "$cfg" | grep -q 'alignment=PAGE_ALIGNMENT_16K'; then
            echo "  ${GREEN}PAGE_ALIGNMENT_16K${RESET}"
        elif echo "$cfg" | grep -q 'alignment=PAGE_ALIGNMENT_4K'; then
            echo "  ${RED}PAGE_ALIGNMENT_4K — Play-generated APKs will not be 16 KB zip-aligned${RESET}"
            FAILED=1
        else
            echo "  ${YELLOW}WARNING: could not confirm PAGE_ALIGNMENT_16K in BundleConfig.pb${RESET}"
            FAILED=1
        fi
        if echo "$cfg" | grep -q 'enabled=0'; then
            echo "  ${RED}uncompress_native_libraries.enabled=0 (legacy compressed packaging)${RESET}"
            echo "  AGP must store uncompressed native libs so Play mmap's them on 16 KB devices."
            FAILED=1
        fi
    fi
fi

echo
echo "=== Summary ==="
echo "  ALIGNED 64-bit libs: $ALIGNED_COUNT"
echo "  skipped 32-bit:      $SKIPPED_32BIT"
if [[ ${#UNALIGNED[@]} -gt 0 ]]; then
    echo "  ${RED}UNALIGNED 64-bit libs: ${#UNALIGNED[@]}${RESET}"
    for lib in "${UNALIGNED[@]}"; do
        echo "    $lib"
    done
    exit 1
fi
if [[ "$FAILED" -ne 0 ]]; then
    echo "  ${RED}16 KB packaging check failed${RESET}"
    exit 1
fi
echo "  ${GREEN}ELF Verification Successful${RESET}"
exit 0
