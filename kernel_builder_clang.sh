#!/bin/bash

# shitty kernel reeeee

PREFIX="/tmp/optane/clang"
CLANG="clang-r614150"

#rm -rf out
#mkdir out
#rm -rf error.log
#make O=out clean 
#make mrproper

# Build

CLANG_DIR=${PREFIX}/${CLANG}
export PATH="$CLANG_DIR/bin:$PATH"
export LD_LIBRARY_PATH="$CLANG_DIR/lib:$CLANG_DIR/lib64:$LD_LIBRARY_PATH"

echo $PATH

make O=out ARCH=arm64 mojito_defconfig

make -j $(nproc) ARCH=arm64 SUBARCH=arm64 O=out \
        CC="ccache clang"\
        AR="llvm-ar" \
	NM="llvm-nm" \
	LD="ld.lld -S" \
	OBJCOPY="llvm-objcopy" \
	OBJDUMP="llvm-objdump" \
	STRIP="llvm-strip" \
        CLANG_TRIPLE="aarch64-linux-gnu-" \
    	CROSS_COMPILE="aarch64-linux-gnu-" \
    	CROSS_COMPILE_ARM32="arm-linux-gnueabi-" \
    	CROSS_COMPILE_COMPAT="arm-linux-gnueabi-" \
    	LLVM=1 \
    	LLVM_IAS=1 \
    	INSTALL_MOD_STRIP=1 \
	KBUILD_BUILD_USER="$(git rev-parse --short HEAD | cut -c1-7)" \
	KBUILD_BUILD_HOST="$(git symbolic-ref --short HEAD)"	
	
ccache -s

# fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp
# for i in $(ls patches/) ; do patch -Np1 < patches/$i ; done
