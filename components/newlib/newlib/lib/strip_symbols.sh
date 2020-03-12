
libs="libc_fnano.a libc.a libc_nano.a"
symbols="lib_a-errno.o"

for symbol in ${symbols}
do
    for lib in ${libs}
    do
        xtensa-lx106-elf-ar d ${lib} ${symbol} 
    done
done
