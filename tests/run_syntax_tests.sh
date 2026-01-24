#!/bin/bash

TESTDIR=./syntax_tests
EXEC=./test_syntax
FAILED=0

# Check arguments.
if [[ $# -ne 0 && $# -ne 1 ]] ; then
    echo "usage: ./run_syntax_tests.sh [-v]"
    exit 1
fi

if [[ $# -eq 1 && $1 != "-v" ]] ; then
    echo "usage: ./run_syntax_tests.sh [-v]"
    exit 1
fi


# Run passing tests and ensure that they pass.
for test in $TESTDIR/pass.* ; do
    $EXEC $test &> /dev/null
    
    # Make sure test passed.
    if [ $? -ne 0 ] ; then
        echo "FAIL: $test"
        FAILED=1
    elif [ $# -eq 1 ] ; then
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
    elif [ $# -eq 1 ] ; then
        echo "PASS: $test"
    fi
done

if [ $FAILED -ne 0 ] ; then
    exit 1
else
    exit 0
fi

