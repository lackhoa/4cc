#!/bin/bash
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
set -e

# TODO: copy the binary out to my actual user directory
PROJECT_ROOT="$(dirname "${HERE}")"
BUILD_MODE="-DOPT_BUILD"
"$HERE/bin/build-mac.sh" "${BUILD_MODE}"

OUPUT_DIR="${HOME}/4coder"

rm -rf "$OUPUT_DIR/4ed.dSYM"
rm -rf "$OUPUT_DIR/4ed_app.so.dSYM"

mv -f "$PROJECT_ROOT/build/4ed"             "$OUPUT_DIR"
mv -f "$PROJECT_ROOT/build/4ed.dSYM"        "$OUPUT_DIR" 2> /dev/null || true
mv -f "$PROJECT_ROOT/build/4ed_app.so"      "$OUPUT_DIR"
mv -f "$PROJECT_ROOT/build/4ed_app.so.dSYM" "$OUPUT_DIR" 2> /dev/null || true
