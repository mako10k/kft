#!/bin/sh

. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

TEXT="hello world"
RESULT="$(timeout 1 kft -e "$TEXT")"
if [ "$RESULT" != "$TEXT" ]; then
    echo "Expected '$TEXT', got '$RESULT'"
    exit 1
fi
exit 0