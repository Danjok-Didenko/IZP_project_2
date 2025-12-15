#!/bin/sh

# Test script for IZP 2025/26 proj2.
# Author: Ales Smrcka
# Date: 2025-12-05
export POSIXLY_CORRECT=y

cd $(dirname $0)

die()
{
    echo "$@" >&2
    exit 1
}

# check if software under test exists
if ! [ -f flows.c ]; then
    die "File flows.c not found. Run $0 in the same directory as the file."
fi

# compile SUT
gcc -std=c11 -Wall -Wextra -pedantic -DNDEBUG -o flows flows.c -lm || die "Compilation failed."

# $1 test id
# $2 file with expected content of stdout
# $3-... SUT argument
# result 0=test passed, 1=test failed
run_test()
{
    local testid="$1"
    local stdoutref="$2"
    local outfile="test$id-stdout.tmp"
    shift 2
    timeout 0.5 ./flows "$@" </dev/zero >$outfile 2>&1
    diff -iBw $stdoutref $outfile >/dev/null
    result=$?
    [ $result -eq 0 ] && rm -f $stdoutref $outfile
    return $result
}

# Colored terminal ouput
GREEN=
RED=
RESET=
if [ -t 1 ]; then 
    GREEN="\033[1;32m"
    RED="\033[1;31m"
    RESET="\033[0m"
fi

# $1 test id
# $2 test description
# $3 expected output
# $4-... SUT arguments
test_case()
{
    local id="$1"
    local desc="$2"
    local expectedfile="test${id}-expected.tmp"
    local expectedcontent="$3"
    shift 3
    printf "$expectedcontent" >"$expectedfile"

    run_test $id "$expectedfile" "$@"
    local result=$?

    if [ $result -eq 0 ]; then
        printf "test $id: ${GREEN}✔${RESET} $desc\n"
    else
        printf "test $id: ${RED}✘${RESET} $desc\n"
        echo "    Execution:"
        echo "        ./flows $@"
        echo "    Expected:"
        sed 's/^/        /' <$expectedfile
    fi
    return $result
}

# -----------------------------
# Setup all
# -----------------------------

cat >data.txt <<EOF
count=4
10 192.168.1.1 192.168.1.2 2000 10 20 0.05
11 10.0.0.5    10.0.0.6    4000 20 30 0.07
12 192.168.2.1 192.168.2.3 1500 8  10 0.03
13 172.16.0.10 172.16.0.11 6000 25 45 0.06
EOF

cat >data2.txt <<EOF
count=4
13 172.16.0.10 172.16.0.11 6000 25 45 0.06
12 192.168.2.1 192.168.2.3 1500 8  10 0.03
11 10.0.0.5    10.0.0.6    4000 20 30 0.07
10 192.168.1.1 192.168.1.2 2000 10 20 0.05
EOF

# clean previous runs
rm -f test*-expected.tmp test*-stdout.tmp 2>/dev/null

# -----------------------------
# Define all test cases
# -----------------------------

test_case 1 "cteni a tisk clusteru" \
"Clusters:
cluster 0: 10
cluster 1: 11
cluster 2: 12
cluster 3: 13
" \
data.txt 4 1 1 1 1

test_case 2 "cteni a serazeny tisk clusteru" \
"Clusters:
cluster 0: 10
cluster 1: 11
cluster 2: 12
cluster 3: 13
" \
data2.txt 4 1 1 1 1

test_case 3 "cteni a serazeny tisk celeho shluku" \
"Clusters:
cluster 0: 10 11 12 13
" \
data.txt 1 1 1 1 1

test_case 4 "cteni a serazeny tisk 2 shluku, priklad ze zadani" \
"Clusters:
cluster 0: 10 12
cluster 1: 11 13
" \
data.txt 2 1 1 1 1

test_case 5 "cteni a serazeny tisk 2 shluku, float vahy" \
"Clusters:
cluster 0: 10 12
cluster 1: 11 13
" \
data.txt 2 1e-6 1.2 234.5 4.2E-3

test_case 6 "cteni a serazeny tisk 3 shluku, vaha na WS" \
"Clusters:
cluster 0: 10
cluster 1: 11 13
cluster 2: 12
" \
data.txt 3 0 0 0 1
