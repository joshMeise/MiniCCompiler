#!/bin/bash

SYNTAX_TESTDIR=./syntax_tests
SYNTAX_EXEC=./test_syntax

# Check arguments.
if [[ $# -ne 0 && $# -ne 1 ]] ; then
    echo "usage: ./run_all_tests.sh [-v]"
    exit 1
fi

if [[ $# -eq 1 && $1 != "-v" ]] ; then
    echo "usage: ./run_all_tests.sh [-v]"
    exit 1
fi

echo "[Syntax Tests]"

# Run passing tests and ensure that they pass.
for test in $SYNTAX_TESTDIR/pass.* ; do
    $SYNTAX_EXEC $test &> /dev/null
    
    # Make sure test passed.
    if [ $? -ne 0 ] ; then
        echo "FAIL: $test"
    elif [ $# -eq 1 ] ; then
        echo "PASS: $test"
    fi
done

# Run failing tests and ensure that they fail.
for test in $SYNTAX_TESTDIR/fail.* ; do
    $SYNTAX_EXEC $test &> /dev/null

    # Make sure test failed.
    if [ $? -eq 0 ] ; then
        echo "FAIL: $test"
    elif [ $# -eq 1 ] ; then
        echo "PASS: $test"
    fi
done

