#!/bin/bash

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

SYNTAX_TESTDIR=./syntax_tests
SYNTAX_EXEC=./test_syntax
SEMANTICS_TESTDIR=./semantics_tests
SEMANTICS_EXEC=./test_semantics
AGEN_TESTDIR=./assembly_gen_tests
AGEN_EXEC=./test_assembly_gen


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

echo "[Semantics Tests]"

# Run passing tests and ensure that they pass.
for test in $SEMANTICS_TESTDIR/pass.* ; do
    $SEMANTICS_EXEC $test &> /dev/null
    
    # Make sure test passed.
    if [ $? -ne 0 ] ; then
        echo "FAIL: $test"
    elif [ $# -eq 1 ] ; then
        echo "PASS: $test"
    fi
done

# Run failing tests and ensure that they fail.
for test in $SEMANTICS_TESTDIR/fail.* ; do
    $SEMANTICS_EXEC $test &> /dev/null

    # Make sure test failed.
    if [ $? -eq 0 ] ; then
        echo "FAIL: $test"
    elif [ $# -eq 1 ] ; then
        echo "PASS: $test"
    fi
done

echo "[Assembly Gen Tests]"

# Generate LLVM files for each test file.
pushd $AGEN_TESTDIR
make clean > /dev/null
make llvm_files > /dev/null
popd

for test in $AGEN_TESTDIR/test*.ll ; do
    outfile=${test%.ll}.s
    $AGEN_EXEC $test $outfile &> /dev/null

    # Make sure assembly code was generated.
    if [ $? -ne 0 ] ; then
        echo "FAILED to generate assembly code for: $test"
    fi
done

# Run all tests.
pushd $AGEN_TESTDIR
make > /dev/null

for exec in ./* ; do
    if [ -x $exec ] ; then
        ./$exec &> /dev/null

        if [ $? -ne 0 ] ; then
            echo "FAIL: $exec"
        elif [ $# -eq 1 ] ; then
            echo "PASS: $exec"
        fi
    fi
done

make clean > /dev/null
popd

