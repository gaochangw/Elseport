#!/bin/bash
# $Id$
# Elseport - repeatable Apple Silicon preview configuration.
# SPDX-License-Identifier: GPL-3.0-only
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"
if [ "$(uname -m)" != arm64 ]; then
    echo 'This preview build targets Apple Silicon.' >&2
    exit 1
fi
if [ -e LocalConfig.kmk ]; then
    echo 'LocalConfig.kmk already exists. Preserve it and use the documented preview configuration.' >&2
    exit 1
fi
if [ -n "${ELSEPORT_QT_PREFIX:-}" ]; then
    if [ ! -d "$ELSEPORT_QT_PREFIX/lib" ] || [ -e prebuild/qt6 ]; then
        echo 'ELSEPORT_QT_PREFIX must be a complete Qt prefix; prebuild/qt6 must not exist.' >&2
        exit 1
    fi
    mkdir -p prebuild
    ln -s "$ELSEPORT_QT_PREFIX" prebuild/qt6
else
    python3 tools/elseport/prepare-qt.py --output prebuild/qt6
fi
if [ -n "${ELSEPORT_MACOS_SDK:-}" ]; then
    sdk="$ELSEPORT_MACOS_SDK"
else
    sdk=$(DEVELOPER_DIR="${DEVELOPER_DIR:-/Library/Developer/CommandLineTools}" xcrun --sdk macosx --show-sdk-path)
fi
DEVELOPER_DIR="${DEVELOPER_DIR:-/Library/Developer/CommandLineTools}" python3 configure.py \
    --with-kbuild-path "$root/kBuild/kBuild" --ose --disable-qt --disable-docs \
    --disable-java --disable-additions --disable-validationkit --disable-opengl \
    --disable-xcode --with-macossdk-path "$sdk"
cat > LocalConfig.kmk <<'CONFIG'
VBOX_WITH_QT := 1
VBOX_WITH_QTGUI := 1
VBOX_PATH_QT := $(PATH_ROOT)/prebuild/qt6
VBOX_PATH_QT_LIB := $(VBOX_PATH_QT)/lib
PATH_SDK_QT6 := $(VBOX_PATH_QT)
PATH_SDK_QT6_LIB := $(VBOX_PATH_QT)/lib
PATH_SDK_QT6_INC := $(VBOX_PATH_QT)/include
PATH_TOOL_QT6_BIN := $(VBOX_PATH_QT)/bin
PATH_TOOL_QT6_LIBEXEC := $(VBOX_PATH_QT)/libexec
VBOX_DEF_MACOSX_VERSION_MIN := 15.0
PATH_SDK_MACOSX150 := $(VBOX_PATH_MACOSX_SDK)
VBOX_GCC_WERR := -Werror -Wno-error=deprecated-pragma
VBOX_WITH_NLS := 1
VBOX_WITH_MAIN_NLS := 1
VBOX_WITH_VBOXMANAGE_NLS := 1
VBOX_WITH_VMSVGA3D :=
VBOX_WITH_VMSVGA3D_DX :=
VBOX_WITH_3D_ACCELERATION :=
VBOX_WITH_DXMT :=
VBOX_WITH_DXVK :=
VBOX_WITHOUT_HARDENING := 1
CONFIG
. ./env.sh
kmk -j"${ELSEPORT_JOBS:-4}" KBUILD_TYPE=release
