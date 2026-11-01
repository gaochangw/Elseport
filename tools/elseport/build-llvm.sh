#!/bin/bash
# $Id$
## @file
# Elseport: build the pinned public LLVM libraries used by the current DXMT integration.
# SPDX-License-Identifier: GPL-3.0-only

set -euo pipefail
elseport_root=$(cd "$(dirname "$0")/../.." && pwd)
elseport_work=${ELSEPORT_TOOLCHAIN_WORK:-"$elseport_root/prebuild/toolchain-build"}
elseport_prefix=${ELSEPORT_LLVM_PREFIX:-"$elseport_root/prebuild/toolchain/llvm-15.0.7"}
elseport_archive="$elseport_work/llvm-project-15.0.7.src.tar.xz"
elseport_hash=8b5fcb24b4128cf04df1b0b9410ce8b1a729cb3c544e6da885d234280dedeac6

for elseport_tool in curl cmake ninja shasum tar; do
    command -v "$elseport_tool" >/dev/null || { echo "Missing prerequisite: $elseport_tool" >&2; exit 2; }
done
mkdir -p "$elseport_work"
if [ ! -f "$elseport_archive" ]; then
    curl --fail --location --proto '=https' --tlsv1.2 \
        --output "$elseport_archive.part" \
        https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-project-15.0.7.src.tar.xz
    elseport_actual=$(shasum -a 256 "$elseport_archive.part" | cut -d ' ' -f 1)
    test "$elseport_actual" = "$elseport_hash" || { echo "LLVM archive checksum mismatch" >&2; exit 1; }
    mv "$elseport_archive.part" "$elseport_archive"
fi
elseport_actual=$(shasum -a 256 "$elseport_archive" | cut -d ' ' -f 1)
test "$elseport_actual" = "$elseport_hash" || { echo "LLVM archive checksum mismatch" >&2; exit 1; }
if [ ! -d "$elseport_work/llvm-project-15.0.7.src" ]; then
    tar -xf "$elseport_archive" -C "$elseport_work" \
        llvm-project-15.0.7.src/llvm llvm-project-15.0.7.src/cmake llvm-project-15.0.7.src/third-party
fi
cmake -S "$elseport_work/llvm-project-15.0.7.src/llvm" -B "$elseport_work/build" -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$elseport_prefix" -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_CXX_FLAGS='-include cstdint' -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_HOST_TRIPLE=arm64-apple-darwin -DLLVM_ENABLE_ASSERTIONS=On \
    -DLLVM_ENABLE_ZSTD=Off -DLLVM_ENABLE_ZLIB=Off -DLLVM_ENABLE_TERMINFO=Off \
    -DLLVM_ENABLE_LIBXML2=Off -DLLVM_ENABLE_LIBEDIT=Off -DLLVM_INCLUDE_TESTS=Off \
    -DLLVM_INCLUDE_BENCHMARKS=Off -DLLVM_INCLUDE_EXAMPLES=Off \
    -DLLVM_TARGETS_TO_BUILD='' -DLLVM_BUILD_TOOLS=Off
cmake --build "$elseport_work/build" --parallel "${ELSEPORT_BUILD_JOBS:-4}"
cmake --install "$elseport_work/build"
printf 'LLVM installed in %s\n' "$elseport_prefix"
