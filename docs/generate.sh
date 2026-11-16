#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Regenerates the pictures in this directory from the program itself.
# Needs a built shuangpin-overlay and ffmpeg. Without a display, set
# QT_QPA_PLATFORM=offscreen (the font will differ).
#
#   docs/generate.sh [path/to/shuangpin-overlay]
set -eu

app=${1:-build/shuangpin-overlay}
docs=$(dirname "$0")
frames=$(mktemp -d)
trap 'rm -rf "$frames"' EXIT

# --- stills -------------------------------------------------------------------
"$app" --export "$docs/chart.png"
"$app" --export "$docs/hint.png" --after u
"$app" --export "$docs/mistake.png" --after nihcup

mkdir -p "$docs/schemes"
"$app" --list-schemes | cut -f1 | while read -r scheme; do
    "$app" --export "$docs/schemes/$scheme.png" --scheme "$scheme"
done

# --- demo.gif -----------------------------------------------------------------
# Types "ni hao shuang pin" in Xiaohe, hitting P for the final of "shuang"
# first and correcting it. '<' is Backspace.
count=0
list=$frames/list.txt
: > "$list"

frame() { # frame <keys typed so far> <key held down, or ''> <seconds>
    count=$((count + 1))
    file=$frames/$(printf '%03d' "$count").png
    "$app" --export "$file" --after "$1" ${2:+--pressed "$2"}
    printf "file '%s'\nduration %s\n" "$file" "$3" >> "$list"
}

frame '' '' 1.0
typed=''
for step in n:0.5 i:0.5 h:0.6 c:0.6 u:1.2 p:1.3 '<:0.7' l:0.6 p:0.6 b:2.2; do
    key=${step%%:*}
    typed=$typed$key
    held=$key
    [ "$key" = '<' ] && held=''
    frame "$typed" "$held" 0.16
    frame "$typed" '' "${step##*:}"
done
# The concat demuxer ignores the last duration unless the file is repeated.
printf "file '%s'\n" "$file" >> "$list"

ffmpeg -loglevel error -y -f concat -safe 0 -i "$list" \
    -vf "scale=1053:-1:flags=lanczos,split[a][b];[a]palettegen=reserve_transparent=1[p];[b][p]paletteuse=dither=none:alpha_threshold=128" \
    -loop 0 "$docs/demo.gif"
