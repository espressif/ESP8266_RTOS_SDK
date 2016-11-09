#!/bin/bash -x

echo "make_lib.sh version 20160307"
echo ""

cd $1
make clean
make COMPILE=gcc
cp .output/eagle/debug/lib/lib$1.a ../../lib/lib$1.a
xtensa-lx106-elf-strip --strip-unneeded ../../lib/lib$1.a
cd ..
