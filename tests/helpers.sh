set -e
PATH="../src:$PATH"

timeout_detector() {
    ret=$?
    if [ "$ret" -eq 124 ]; then
        echo "$TESTMSG is timeouted"
        exit 1
    else
        echo "$TESTMSG is failed"
    fi
    exit $ret
}
trap timeout_detector EXIT

run_expect() {
    EXPECT="$1"
    shift
    TESTMSG="$*"
    RESULT="$(timeout 1 "$@")"
    if [ "$RESULT" != "$EXPECT" ]; then
        echo "Expected '$EXPECT', got '$RESULT'"
        exit 1
    fi
}