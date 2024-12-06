#!/bin/bash
set -e

. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

if ! tty > /dev/null; then
    echo "Skipping test, not running in a tty"
    exit 77
fi

run() {
    if tty > /dev/null; then
        EXPECT_READTTY=124
        EXPECT_NOREADTTY=0
        MSG=" within a TTY"
    else
        EXPECT_READTTY=0
        EXPECT_NOREADTTY=0
        MSG=" outside a TTY"
    fi

    set +e

    timeout 1 kft > /dev/null
    if [ $? -ne $EXPECT_READTTY ]; then
        echo "wrong TTY reading with no arguments$MSG"
        exit 1
    fi

    timeout 1 kft -e '{{$DUMMY}}' > /dev/null
    if [ $? -ne $EXPECT_NOREADTTY ]; then
        echo "wrong TTY reading with no arguments$MSG"
        exit 1
    fi

    timeout 1 kft DUMMY="$TEXT" -e '{{$DUMMY}}' > /dev/null
    if [ $? -ne $EXPECT_NOREADTTY ]; then
        echo "wrong TTY reading with no arguments$MSG"
        exit 1
    fi
}


run
run < /dev/null
