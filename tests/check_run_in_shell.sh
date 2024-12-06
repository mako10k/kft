#!/bin/sh

. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

TESTMSG="kft -e \"{{!echo '$TEXT'}}\""
RESULT="$(timeout 1 kft -e "{{!echo '$TEXT'}}")"
if [ "$RESULT" != "$TEXT" ]; then
    echo "Expected '$TEXT', got '$RESULT'"
    exit 1
fi
exit 0