#!/bin/bash
set -e
TEXT="hello world"
RESULT="$(timeout 10 kft ENV="${TEST}" -e '{{$ENV}}')"
if [ "$RESULT" != "$TEXT" ]; then
    echo "Expected '$TEXT', got '$RESULT'"
    exit 1
fi