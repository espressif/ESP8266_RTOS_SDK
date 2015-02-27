#!/bin/bash -x
make
if [ $? == 0 ];then
rm ../bin/eagle.app.v6.flash.bin ../bin/eagle.app.v6.irom0text.bin ../bin/eagle.app.v6.dump ../bin/eagle.app.v6.S

cd .output/eagle/debug/image

if xtensa-lx106-elf-objcopy --version 2>/dev/null ; then
    CROSS_COMPILE=xtensa-lx106-elf-
fi
export CROSS_COMPILE
CROSS_COMPILE=${CROSS_COMPILE:-xt-}

${CROSS_COMPILE}objdump -x -s eagle.app.v6.out > ../../../../../bin/eagle.app.v6.dump
${CROSS_COMPILE}objdump -S eagle.app.v6.out > ../../../../../bin/eagle.app.v6.S

${CROSS_COMPILE}objcopy --only-section .text -O binary eagle.app.v6.out eagle.app.v6.text.bin
${CROSS_COMPILE}objcopy --only-section .data -O binary eagle.app.v6.out eagle.app.v6.data.bin
${CROSS_COMPILE}objcopy --only-section .rodata -O binary eagle.app.v6.out eagle.app.v6.rodata.bin
${CROSS_COMPILE}objcopy --only-section .irom0.text -O binary eagle.app.v6.out eagle.app.v6.irom0text.bin

../../../../../tools/gen_appbin.py eagle.app.v6.out v6

cp eagle.app.v6.irom0text.bin ../../../../../bin/
cp eagle.app.v6.flash.bin ../../../../../bin/

cd ../../../../../

else
echo "make error"
fi
