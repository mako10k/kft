#!/bin/sh
. "$(dirname "$0")/helpers.sh"

TEXT="hello world"

run_expect "$TEXT" kft -e "$TEXT"

exit 0