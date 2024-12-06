#!/bin/sh
set -e
TEXT="hello world"
RESULT="$(timeout 10 kft -e "$TEXT")"
if [ "$RESULT" != "$TEXT" ]; then
    echo "Expected '$TEXT', got '$RESULT'"
    exit 1
fi
exit 0