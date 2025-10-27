#!/bin/bash

########################################################
# ~/compiler_src/
#  ├── gcc-15.2.0/           # GCC 源码
#  ├── glibc-2.36/           # glibc 源码
#  ├── gdb-16.3/             # GDB 源码
#  ├── build-gcc-stage1/     # 阶段1: 初始 GCC 构建目录
#  ├── build-glibc/          # 阶段2: glibc 构建目录
#  ├── build-gcc-final/      # 阶段3: 最终 GCC 构建目录
#  ├── build-gdb/            # GDB 构建目录
#  └── build_compiler.sh     # 构建脚本
########################################################

WORKDIR=$(pwd)
PREFIX=$HOME/.compiler
SYSROOT=$PREFIX/sysroot

# 创建或者清空工作目录
rm -rf "${WORKDIR:?}"/* || exit
rm -rf "${PREFIX:?}"/* || exit
mkdir -p "$WORKDIR"
mkdir -p "$PREFIX"

# 下载源码
cd "$WORKDIR" || exit
# shellcheck disable=SC2046
wget https://mirrors.aliyun.com/gnu/glibc/glibc-2.36.tar.gz
wget https://mirrors.aliyun.com/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.gz
wget https://mirrors.aliyun.com/gnu/gdb/gdb-16.3.tar.gz
git clone https://mirrors.tuna.tsinghua.edu.cn/git/linux.git

# 解压源码
tar -zxvf glibc-2.36.tar.gz
tar -zxvf gcc-15.2.0.tar.gz
tar -zxvf gdb-16.3.tar.gz

# 切换到 linux 5.15 版本，安装linux-headers
cd "$WORKDIR"/linux && git checkout v5.15
rm -rf "${SYSROOT:?}"/usr/* || exit
make headers_install INSTALL_HDR_PATH="$SYSROOT"/usr || exit

# 下载依赖
cd "$WORKDIR"/gcc-15.2.0/ && ./contrib/download_prerequisites || exit

### 阶段1: 最小化构建gcc
mkdir -p "$WORKDIR"/build-gcc-stage1 && cd "$WORKDIR"/build-gcc-stage1 || exit
rm -rf "$WORKDIR"/build-gcc-stage1/* && rm -rf "$PREFIX"/gcc-15.2.0-stage1/* || exit
"$WORKDIR"/gcc-15.2.0/configure \
    --prefix="$PREFIX"/gcc-15.2.0-stage1 \
    --target=x86_64-linux-gnu \
    --disable-multilib \
    --enable-languages=c \
    --disable-werror \
    --without-headers \
    --disable-nls \
    --with-system-zlib=no \
    --enable-bootstrap

# shellcheck disable=SC2046
make -j$(nproc) && make install || exit

"$PREFIX"/gcc-15.2.0-stage1/bin/x86_64-linux-gnu-gcc -v || exit


### 阶段2: 构建glibc
mkdir -p "$WORKDIR"/build-glibc && cd "$WORKDIR"/build-glibc || exit
rm -rf "$WORKDIR"/build-glibc/* || exit
"$WORKDIR"/glibc-2.36/configure \
    --prefix="$SYSROOT"/usr \
    --disable-werror \
    --enable-static-nss \
    --disable-nscd \
    --host=x86_64-linux-gnu \
    --build=x86_64-linux-gnu \
    --with-headers="$SYSROOT"/usr/include \
    CC="${PREFIX}/gcc-15.2.0-stage1/bin/x86_64-linux-gnu-gcc --sysroot=${SYSROOT} -B${PREFIX}/gcc-15.2.0-stage1/lib/gcc/x86_64-linux-gnu/15.2.0"

# shellcheck disable=SC2046
make -j$(nproc) && make install || exit

ls "$SYSROOT"/usr/lib || exit
ls "$SYSROOT"/usr/include || exit


# ### 阶段3: 构建最终gcc
mkdir -p "$WORKDIR"/build-gcc-final && rm -rf "$WORKDIR"/build-gcc-final/* && cd "$WORKDIR"/build-gcc-final || exit
"$WORKDIR"/gcc-15.2.0/configure \
    --prefix="$PREFIX"/gcc-15.2.0 \
    --target=x86_64-linux-gnu \
    --disable-multilib \
    --enable-languages=c,c++ \
    --disable-werror \
    --enable-bootstrap \
    --with-system-zlib=no \
    --with-build-sysroot="$SYSROOT" \
    --with-sysroot="$SYSROOT" \
    --disable-nls

# shellcheck disable=SC2046
make -j$(nproc) && make install || exit

"$PREFIX"/gcc-15.2.0/bin/gcc -print-sysroot || exit
# 输出应该是: ../sysroot



### 阶段4: 构建gdb
mkdir -p "$WORKDIR"/build-gdb && cd "$WORKDIR"/build-gdb || exit
rm -rf "$WORKDIR"/build-gdb/* || exit
"$WORKDIR"/gdb-16.3/configure \
    --prefix="$PREFIX"/gdb-16.3 \
    --with-expat \
    --with-lzma \
    --with-system-zlib=no \
    --disable-werror

# shellcheck disable=SC2046
make -j$(nproc) && make install || exit

"$PREFIX"/gdb-16.3/bin/gdb -v || exit


### 阶段5: 测试
export PATH=$HOME/.local/gcc-15.2.0/bin:$PATH

echo '#include <stdio.h>' > hello.c
echo 'int main(){puts("hello");}' >> hello.c

gcc hello.c -o hello
./hello
# 输出：hello
