#!/bin/bash

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

TESTDIR=./integration_tests
EXEC=../execs/compiler
FAILED=0

# Check arguments.
if [[ $# -ne 0 && $# -ne 1 ]] ; then
    echo "usage: ./run_integration_tests.sh [-v]"
    exit 1
fi

if [[ $# -eq 1 && $1 != "-v" ]] ; then
    echo "usage: ./run_integration_tests.sh [-v]"
    exit 1
fi

for test in $TESTDIR/test*.c ; do
    outfile=${test%.c}.s
    $EXEC $test $outfile &> /dev/null

    # Make sure assembly code was generated.
    if [ $? -ne 0 ] ; then
        echo "FAILED to build assembly code for: $test"
        FAILED=1
    fi
done

# Run all tests.
pushd $TESTDIR
make > /dev/null

for exec in ./* ; do
    if [ -x $exec ] ; then
        ./$exec &> /dev/null

        if [ $? -ne 0 ] ; then
            echo "FAIL: $exec"
            FAILED=1
        elif [ $# -eq 1 ] ; then
            echo "PASS: $exec"
        fi
    fi
done

make clean > /dev/null
popd

if [ $FAILED -ne 0 ] ; then
    exit 1
else
    exit 0
fi

