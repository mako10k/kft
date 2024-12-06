#!/bin/bash

. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

TESTMSG="kft ENV=${TEXT} -e '{{\$ENV}}'"
RESULT="$(timeout 1 kft ENV="${TEXT}" -e '{{$ENV}}')"
if [ "$RESULT" != "$TEXT" ]; then
    echo "Expected '$TEXT', got '$RESULT'"
    exit 1
fi

TESTMSG="kft ENV=${TEXT} -e '{{\$ENV}}'"
ENV="${TEXT}"
RESULT="$(timeout 1 kft ENV= -e '{{$ENV}}')"
if [ "$RESULT" != "" ]; then
    echo "Expected '', got '$RESULT'"
    exit 1
fi