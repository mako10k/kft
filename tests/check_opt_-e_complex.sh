#!/bin/sh
set -e
TEXT="hello world"
RESULT="$(timeout 10 kft -e "$TEXT" -e "$TEXT")"
if [ "$RESULT" != "$TEXT$TEXT" ]; then
    echo "Expected '$TEXT$TEXT', got '$RESULT'"
    exit 1
fi
exit 0