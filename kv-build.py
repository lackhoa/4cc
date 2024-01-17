#!/usr/bin/env python3 -u

import os
import subprocess
from subprocess import PIPE, STDOUT
import sys
import time

def run(command, update_env={}):
    # print(command)
    begin = time.time()
    env = os.environ.copy()
    env.update(update_env)
    process = subprocess.run(command, shell=True, capture_output=True, env=env)

    if len(process.stdout):
        print(process.stdout.decode("utf-8"))
    if len(process.stderr):
        print(process.stderr.decode("utf-8"))

    if process.returncode == 0:
        end = time.time()
        if (end - begin) > 0.05:
            print(f'Time taken: {end - begin:.3f} seconds')
            print()
    else:
        exit(1)

    return process

def mtime(path):
    try:
        return os.path.getmtime(path)
    except:
        return 0

# cd to script directory
HOME=os.path.expanduser("~")
HERE = os.path.dirname(os.path.realpath(__file__))
FCODER_USER=f'{HOME}/4coder'  # NOTE: for debug
FCODER_ROOT=f'{HOME}/4ed'
CUSTOM=f'{HOME}/4ed/code/custom'
AUTODRAW=f'{HOME}/AutoDraw'
SOURCE=f'{HERE}/4coder_kv.cpp'
DEBUG_MODE = True

try:
    os.chdir(f'{FCODER_ROOT}')
    print(f'Workdir: {os.getcwd()}')

    run_only       = (len(sys.argv) > 1 and sys.argv[1] == 'run')
    full_rebuild   = (len(sys.argv) > 1 and sys.argv[1] == 'full')  # hopefully never have to be used

    ADDRESS_SANITIZER_ON = False

    INCLUDES=f'-I{HERE}/custom_patch -I{CUSTOM} -I{AUTODRAW}/libs -I{AUTODRAW}/4coder_kv/libs -I{AUTODRAW}/code'
    arch = "-m64"
    debug="-g" if DEBUG_MODE else ""
    COPY_TO_STABLE = False

    if run_only:
        dyld_insert_libraries="DYLD_INSERT_LIBRARIES=/usr/local/Cellar/llvm/17.0.6/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" if ADDRESS_SANITIZER_ON else ""
        run(f'{dyld_insert_libraries} {FCODER_USER}/4ed > /dev/null')
    else:
        print('NOTE: Building the 4ed binary')
        BUILD_MODE = "" if DEBUG_MODE else "-DOPT_BUILD"
        run(f'{HERE}/bin/build-mac.sh {BUILD_MODE}')
        #
        OUTDIR = FCODER_USER if DEBUG_MODE else f"{HOME}/4coder_stable"
        print(f'NOTE: copy the binary out to {OUTDIR}')
        #
        run(f'mv -f "{FCODER_ROOT}/build/4ed"         "{OUTDIR}"')
        run(f'mv -f "{FCODER_ROOT}/build/4ed_app.so"  "{OUTDIR}"')
        if DEBUG_MODE:
            run(f'rm -rf "{FCODER_USER}/4ed.dSYM"')
            run(f'rm -rf "{FCODER_USER}/4ed_app.so.dSYM"')
            run(f'mv -f "{FCODER_ROOT}/build/4ed.dSYM"        "{FCODER_USER}" 2> /dev/null || true')
            run(f'mv -f "{FCODER_ROOT}/build/4ed_app.so.dSYM" "{FCODER_USER}" 2> /dev/null || true')

        print('Build complete!')

except Exception as e:
    print(f'Error: {e}')
