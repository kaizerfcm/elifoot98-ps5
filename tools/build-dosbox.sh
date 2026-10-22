#!/usr/bin/env bash
# ps5-native-app-boilerplate - DOSBox Pure static library for Elifoot 98.
# Copyright (C) 2026 BlackBearReloaded
# SPDX-License-Identifier: GPL-3.0-or-later
#
# DOSBox Pure itself is GPL-2.0-or-later. This script only fetches and compiles
# the pinned revision with the template toolchain.

set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
commit=cdaea10ce8cac231bc341ca05d4c5e4e5fe0fbd5
dest="$root/.deps/dosbox-pure"
archive="$dest/libdosbox_pure.a"

bash "$root/tools/setup-native-dependencies.sh" >/dev/null
sdk="$root/.deps/native/ps5-payload-sdk"
[[ -x $sdk/bin/prospero-lld ]] || {
    echo "the PS5 payload SDK is not available" >&2
    exit 2
}

if [[ ! -f $dest/Makefile ]]; then
    rm -rf -- "$dest"
    mkdir -p -- "$dest"
    git init -q "$dest"
    git -C "$dest" remote add origin https://github.com/libretro/dosbox-pure.git
    git -C "$dest" fetch --depth 1 -q origin "$commit"
    git -C "$dest" -c advice.detachedHead=false checkout --detach -q FETCH_HEAD
fi

wrapper="$dest/prospero-cxx.sh"
cat >"$wrapper" <<EOF
#!/bin/sh
export PS5_PAYLOAD_SDK="$sdk"
exec sh "$root/tooling/prospero-clang18" "\$@"
EOF
chmod +x "$wrapper"

ar=$(command -v llvm-ar-18 || command -v llvm-ar || true)
[[ -n $ar ]] || {
    echo "llvm-ar is required to archive DOSBox Pure" >&2
    exit 2
}

make -C "$dest" -j"$(nproc 2>/dev/null || printf '2')" \
    CXX="$wrapper" CC="$wrapper" AR="$ar" \
    STRIP=true STATIC_LINKING=1 COMMONFLAGS="-fPIC -frtti" \
    OUTNAME=libdosbox_pure.a BUILD=RELEASE
[[ -f $archive ]] || {
    echo "DOSBox Pure did not produce $archive" >&2
    exit 2
}
