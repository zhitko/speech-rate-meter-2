#!/usr/bin/env bash
# Convert docs/SpeechRateMeter2_UserGui*.md to PDF using Pandoc and LuaLaTeX in Docker.
set -euo pipefail

DOCS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCKER_IMAGE="${PANDOC_IMAGE:-pandoc/latex:latest}"
HOST_FONTS_DIR="${FONTS_DIR:-/usr/share/fonts/truetype/dejavu}"

usage() {
    cat <<'EOF'
Usage: ./docs/convert_user_guide.sh [--pdf] [--help]

Convert every docs/SpeechRateMeter2_UserGui*.md to a PDF next to its source file.
Can be run from any directory. Existing PDFs are overwritten.

Requirements:
  Docker with a running daemon (Pandoc and LaTeX run in the container).
  DejaVu fonts for Latin and Cyrillic text (fonts-dejavu-core on Debian/Ubuntu).
  Screenshots referenced by the guides under docs/images/.

Environment overrides:
  FONTS_DIR     Directory containing DejaVu font files.
  PANDOC_IMAGE  Docker image (default: pandoc/latex:latest).
EOF
}

for arg in "$@"; do
    case "$arg" in
        --pdf) ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Error: unknown option: $arg" >&2; usage >&2; exit 1 ;;
    esac
done

if ! command -v docker &>/dev/null; then
    echo "Error: docker is not installed or not in PATH." >&2
    exit 1
fi

if [[ ! -d "$HOST_FONTS_DIR" ]]; then
    echo "Error: DejaVu fonts not found: $HOST_FONTS_DIR" >&2
    echo "Install fonts-dejavu-core or set FONTS_DIR to a DejaVu font directory." >&2
    exit 1
fi
HOST_FONTS_DIR="$(cd "$HOST_FONTS_DIR" && pwd)"

shopt -s nullglob
MD_FILES=("$DOCS_DIR"/SpeechRateMeter2_UserGui*.md)
if [[ ${#MD_FILES[@]} -eq 0 ]]; then
    echo "Error: no SpeechRateMeter2_UserGui*.md files found in $DOCS_DIR" >&2
    exit 1
fi

if ! docker info >/dev/null; then
    echo "Error: cannot connect to Docker. Start the daemon and check access permissions." >&2
    exit 1
fi
if ! docker image inspect "$DOCKER_IMAGE" &>/dev/null; then
    echo "Pulling Docker image $DOCKER_IMAGE ..."
    docker pull "$DOCKER_IMAGE"
fi

for md in "${MD_FILES[@]}"; do
    name="$(basename "$md")"
    title="$(sed -n '/^# /{s/^# //;p;q;}' "$md")"
    title="${title:-${name%.md}}"
    toc_title="Table of Contents"
    if [[ "$name" == *_ru.md ]]; then
        toc_title="Содержание"
    fi

    echo "Converting $name to ${name%.md}.pdf ..."
    docker run --rm \
        --user "$(id -u):$(id -g)" \
        -e HOME=/tmp \
        -e XDG_CACHE_HOME=/tmp/.cache \
        -v "$DOCS_DIR:/data:rw" \
        -v "$HOST_FONTS_DIR:/usr/share/fonts/truetype/dejavu:ro" \
        --workdir /data \
        --entrypoint sh \
        "$DOCKER_IMAGE" \
        -c 'mkdir -p /tmp/.cache/fontconfig && fc-cache -f >/dev/null 2>&1; exec pandoc "$@"' \
        -- \
        "/data/$name" \
        --from markdown \
        --resource-path /data \
        --lua-filter /data/scale_user_guide_images.lua \
        --standalone \
        --toc \
        --toc-depth=3 \
        --metadata "title=$title" \
        --metadata "toc-title=$toc_title" \
        --pdf-engine=lualatex \
        -V mainfont="DejaVu Serif" \
        -V sansfont="DejaVu Sans" \
        -V monofont="DejaVu Sans Mono" \
        -V fontsize=11pt \
        -V geometry:margin=1in \
        -V colorlinks=true \
        -V linkcolor=blue \
        -o "/data/${name%.md}.pdf"
    echo "  -> $DOCS_DIR/${name%.md}.pdf"
done

echo "Done."
