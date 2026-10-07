#!/usr/bin/env bash
# =============================================================================
# clean_android_build.sh — Remove all Android build directories
#
# Usage:
#   chmod +x scripts/clean_android_build.sh
#   ./scripts/clean_android_build.sh            # interactive (prompts for confirmation)
#   ./scripts/clean_android_build.sh --force    # skip confirmation
#   ./scripts/clean_android_build.sh --dry-run  # only list what would be removed
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DRY_RUN=false
FORCE=false

for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        --force)   FORCE=true   ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [--force] [--dry-run]"
            exit 1
            ;;
    esac
done

# Collect all build_android_* directories in the project root
BUILD_DIRS=()
while IFS= read -r -d '' d; do
    BUILD_DIRS+=("$d")
done < <(find "$PROJECT_ROOT" -maxdepth 1 -type d -name 'build_android_*' -print0 | sort -z)

if [[ ${#BUILD_DIRS[@]} -eq 0 ]]; then
    echo "No Android build directories found in $PROJECT_ROOT."
    exit 0
fi

echo "The following Android build directories will be REMOVED:"
echo ""
for d in "${BUILD_DIRS[@]}"; do
    size="$(du -sh "$d" 2>/dev/null | cut -f1)"
    echo "  • $d  ($size)"
done
echo ""

if [[ "$DRY_RUN" == true ]]; then
    echo "Dry-run mode — nothing was deleted."
    exit 0
fi

if [[ "$FORCE" == false ]]; then
    read -r -p "Are you sure you want to delete these directories? [y/N] " response
    case "$response" in
        [yY]|[yY][eE][sS]) ;;
        *)
            echo "Aborted."
            exit 1
            ;;
    esac
fi

echo "Deleting..."
for d in "${BUILD_DIRS[@]}"; do
    echo "  rm -rf $d"
    rm -rf "$d"
done

echo ""

count="${#BUILD_DIRS[@]}"
if [[ "$count" -eq 1 ]]; then
    plural="y"
else
    plural="ies"
fi

echo "Done. Removed $count Android build director${plural}."