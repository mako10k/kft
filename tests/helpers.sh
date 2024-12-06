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

