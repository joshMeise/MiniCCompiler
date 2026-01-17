#!/bin/bash

TESTDIR=./tests
EXEC=./syntax
FAILED=0

# Check no arguments.
if [ $# -ne 0 ] ; then
    echo "usage: ./run_tests.sh"
    exit 1
fi

# Run passing tests and ensure that they pass.
for test in $TESTDIR/pass.* ; do
    $EXEC $test &> /dev/null
    
    # Make sure test passed.
    if [ $? -ne 0 ] ; then
        echo "FAIL: $test"
        FAILED=1
    else
        echo "PASS: $test"
    fi
done

# Run failing tests and ensure that they fail.
for test in $TESTDIR/fail.* ; do
    $EXEC $test &> /dev/null

    # Make sure test failed.
    if [ $? -eq 0 ] ; then
        echo "FAIL: $test"
        FAILED=1
    else
        echo "PASS: $test"
    fi
done

if [ $FAILED -ne 0 ] ; then
    exit 1
else
    exit 0
fi

