#!/bin/bash

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

TESTDIR=./optimization_tests
EXEC=./test_optimizations
FAILED=0

# Check arguments.
if [[ $# -ne 0 && $# -ne 1 ]] ; then
    echo "usage: ./run_optimization_tests.sh [-v]"
    exit 1
fi

if [[ $# -eq 1 && $1 != "-v" ]] ; then
    echo "usage: ./run_optimization_tests.sh [-v]"
    exit 1
fi

# Generate LLVM files for each test file.
pushd $TESTDIR
make clean > /dev/null
make llvm_files > /dev/null
popd

for test in $TESTDIR/test*.ll ; do
    $EXEC $test $test &> /dev/null

    # Make sure optimized code was generated.
    if [ $? -ne 0 ] ; then
        echo "FAILED to optimize code for: $test"
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

