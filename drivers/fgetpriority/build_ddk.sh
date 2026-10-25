#!/usr/bin/env bash

export DDK_ROOT=/opt/ddk
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
export LLVM=1
export LLVM_IAS=1

MODULE_DIR=$PWD/../module
export KO_INSTALL_DIR=$MODULE_DIR/ko
rm -rf $KO_INSTALL_DIR &>/dev/null || true
mkdir $KO_INSTALL_DIR &>/dev/null || true

ORIGINAL_PATH=$PATH

KMI_CLANG=(
    android12-5.10:clang-r416183b
    android13-5.10:clang-r450784e
    android13-5.15:clang-r450784e
    android14-5.15:clang-r487747c
    android14-6.1:clang-r487747c
    android15-6.6:clang-r510928
    android16-6.12:clang-r536225
    android17-6.18:clang-r584948c
)

set -e

for pair in "${KMI_CLANG[@]}"
do
    export KMI=${pair%:*}
    export KDIR=$DDK_ROOT/kdir/$KMI
    export CLANG_PATH=$DDK_ROOT/clang/${pair#*:}/bin
    export PATH=$CLANG_PATH:$ORIGINAL_PATH

    make -f Makefile.ddk clean
    make -f Makefile.ddk modules
    make -f Makefile.ddk clean
done

cd $MODULE_DIR

VERSION=$(grep version= module.prop | cut -f2 -d=)

rm -f fgetpriority*.zip
zip -r fgetpriority_$VERSION.zip *
