#!/usr/bin/env bash
# ps5-native-app-boilerplate - Stage the Elifoot disk into read-only assets.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The disk image is not stored in git. A local extract is copied when present.

set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
dest="$root/assets/elifoot98"

if [[ -f $dest/dosbox.conf && -f $dest/hdd.img && -f $dest/d/eli98/ELIFOOT.EXE ]]; then
    printf 'Elifoot assets already staged.\n'
    exit 0
fi

candidates=()
[[ -n ${ELIFOOT_DATA:-} ]] && candidates+=("$ELIFOOT_DATA")
candidates+=(
    "$root/../elifoot98ps5/ps5/dist/elifoot98-data"
    "/elifoot/ps5/dist/elifoot98-data"
)

source=""
for candidate in "${candidates[@]}"; do
    if [[ -f $candidate/dosbox.conf && -f $candidate/hdd.img && -f $candidate/d/eli98/ELIFOOT.EXE ]]; then
        source=$candidate
        break
    fi
done

if [[ -z $source ]]; then
    printf 'No Elifoot extract found. The package will not contain the game disk.\n' >&2
    exit 0
fi

mkdir -p -- "$dest"
cp -a "$source/." "$dest/"
rm -f -- "$dest/hdd.img.orig"
printf 'Staged Elifoot assets from %s\n' "$source"
