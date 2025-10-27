# 源码编译gcc工具链

前提：系统上已经存在一套编译工具链

## 源码下载

```
wget https://mirrors.aliyun.com/gnu/glibc/glibc-2.42.tar.gz
wget https://mirrors.aliyun.com/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.gz
wget https://mirrors.aliyun.com/gnu/gdb/gdb-16.3.tar.gz
```

## 初次构建gcc

```
tar -zxvf gcc-15.2.0.tar.gz
cd gcc-15.2.0
./contrib/download_prerequisites
mkdir build && cd build
../configure \
    --prefix=$HOME/.local/gcc-15.2.0 \
    --enable-languages=c,c++ \
    --disable-multilib \
    --with-system-zlib=no \
    --enable-bootstrap
make -j$(nproc) && make install
```

## 编译glibc

```
tar -zxvf glibc-2.42.tar.gz
cd glibc-2.42
mkdir build && cd build
../configure \
    --prefix=$HOME/.local \
    --disable-werror \
    --disable-nscd
    # --enable-static-nss
make -j$(nproc) && make install
```

## 再次编译gcc

如果你希望$PREFIX/bin/gcc生成的程序默认就链接到你的新 glibc（无需运行时指定 loader），应该重新配置并编译 GCC，使其 --with-sysroot 或默认查找路径指向你的 $PREFIX/glibc。这一步较复杂（涉及 libstdc++、libgcc 的重建）

```
cd gcc-15.2.0
rm -rf build/* && cd build
../configure \
    --prefix=$HOME/.local/gcc-15.2.0 \
    --enable-languages=c,c++ \
    --disable-multilib \
    --with-system-zlib=no \
    --with-sysroot=$HOME/.local \
    --enable-bootstrap
make -j$(nproc) && make install
```