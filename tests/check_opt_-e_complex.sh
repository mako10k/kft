#!/bin/sh
. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

run_expect "$TEXT$TEXT" kft -e "$TEXT" -e "$TEXT"

exit 0