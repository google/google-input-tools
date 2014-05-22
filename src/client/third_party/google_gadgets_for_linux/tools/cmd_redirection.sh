#!/bin/sh

# This script redirects stdout to specified file. We use this script to blame gyp for
# lacking of redirection.

# Usage: cmd_redirection.sh <output_file> <cmd> [arg1, arg2, ...]

if [[ $# < 2 ]]; then
    exit 1
fi

output_file=$1
cmd=$2

shift 2

"$cmd" "$@" > "$output_file"
