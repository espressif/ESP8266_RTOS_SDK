#! /bin/sh

if test "x$TESTS" = "xyes" -o "x$TESTS" = "xtrue" ; then
    WITH_TESTS="`scripts/fix-cunit.sh` --enable-tests"
    test -f `pwd`/cunit.pc && echo cat `pwd`/cunit.pc
fi
     
config() {
    echo "./configure $*"
    ./configure $* || cat config.log
}

case "${PLATFORM}" in
    contiki) config "--disable-tests --disable-documentation --disable-examples" && \
               make -C examples/contiki
             ;;
    lwip)    config "--disable-tests --disable-documentation --disable-examples" && \
               make -C examples/lwip lwip lwip-contrib
               make -C examples/lwip LDLIBS=`grep ac_cv_search_clock_gettime=- config.log|cut -d= -f2`
             ;;
    posix|*) config "$WITH_TESTS --enable-documentation --enable-examples" && \
               make
             ;;
esac

err=$?
if test $err = 0 -a -n "$WITH_TESTS" ; then
    tests/testdriver
    err=$?
fi

exit $err
