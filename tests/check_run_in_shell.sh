#!/bin/sh
. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

run_expect "$TEXT" kft -e "{{!echo '$TEXT'}}"

exit 0