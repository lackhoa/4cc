#!/bin/bash
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $HERE
FCODER_ROOT="$(realpath ../../..)"

function g {
    SEARCH="${1}"
    LOCATION="${2}"
    if [ -z "${SEARCH}" ]; then
        echo "typo"
        exit 1
    fi

    if [ -z "${LOCATION}" ]; then
        LOCATION="$(pwd)"
    fi

    grep -rI --color=auto --exclude-dir=".git" "${SEARCH}" "${LOCATION}"
}

g "TODO(kv)" ${FCODER_ROOT}/code | grep -v 'ignore_todo'

exit 0