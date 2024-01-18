#!/usr/bin/env python3 -u

import os
import subprocess
from subprocess import PIPE, STDOUT
import sys
import time

DEBUG_MODE = True
HOME = os.path.expanduser("~")
FCODER_USER=f'{HOME}/4coder'  # NOTE: for debug build
FCODER_ROOT=f'{HOME}/4ed'

def run(command, update_env={}):
    print(command)
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
            print(f'Time taken: {end - begin:.3f} seconds\n')
    else:
        exit(1)

    return process

def mtime(path):
    try:
        return os.path.getmtime(path)
    except:
        return 0

script_begin = time.time()

try:
    OUTDIR = FCODER_USER if DEBUG_MODE else f"{HOME}/4coder_stable"
    os.chdir(f'{OUTDIR}')
    print(f'Workdir: {os.getcwd()}')

    run_only       = (len(sys.argv) > 1 and sys.argv[1] == 'run')
    full_rebuild   = (len(sys.argv) > 1 and sys.argv[1] == 'full')

    ADDRESS_SANITIZER_ON = False

    arch = "-m64"
    debug="-g" if DEBUG_MODE else ""

    if run_only:
        dyld_insert_libraries="DYLD_INSERT_LIBRARIES=/usr/local/Cellar/llvm/17.0.6/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" if ADDRESS_SANITIZER_ON else ""
        run(f'{dyld_insert_libraries} {FCODER_USER}/4ed > /dev/null')
    else:
        if full_rebuild:  # do some generation business in the custom layer
            autogen_file = f'{FCODER_ROOT}/code/4coder_kv/autogen.py'
            exec( open(autogen_file).read() )

        print('NOTE: Building the 4ed binary')

        # BUILD_MODE = "" if DEBUG_MODE else "-DOPT_BUILD"
        # run(f'{FCODER_ROOT}/code/bin/build-mac.sh {BUILD_MODE}')  # todo(kv): removeme (keeping here just fore reference)

        print('Producing 4ed_app.so')
        WARNINGS="-Wno-write-strings -Wno-null-dereference -Wno-comment -Wno-switch -Wno-missing-declarations -Wno-logical-op-parentheses -Wno-deprecated-declarations -Wno-tautological-compare -Wno-unused-result -Wno-nullability-completeness"
        INCLUDES=f'-I{FCODER_ROOT}/code -I{FCODER_ROOT}/code/custom -I{FCODER_ROOT}/4coder-non-source/foreign/freetype2 -I{FCODER_ROOT}/code/4coder_kv -I{FCODER_ROOT}/code/4coder_kv/libs'
        SYMBOLS="-DKV_SLOW=1 -DKV_INTERNAL=1 -DFRED_INTERNAL -DFTECH_64_BIT -DFRED_SUPER" if DEBUG_MODE else "-DFRED_SUPER -DFTECH_64_BIT"
        OPTIMIZATION_LEVEL="-O0" if DEBUG_MODE else "-O3"
        COMPILE_FLAGS=f"{WARNINGS} {INCLUDES} {SYMBOLS} {OPTIMIZATION_LEVEL} -g -m64 -std=c++11"
        run(f'ccache clang++ {COMPILE_FLAGS} -c {FCODER_ROOT}/code/4ed_app_target.cpp -o 4ed_app.so.o')
        run(f'clang++ -shared 4ed_app.so.o -o 4ed_app.so')

        print('Producing 4ed')
        run(f'ccache clang++ {COMPILE_FLAGS} -I{FCODER_ROOT}/code/platform_all -c "{FCODER_ROOT}/code/platform_mac/mac_4ed.mm" -o 4ed.o')
        LINKED_LIBS=f"{FCODER_ROOT}/4coder-non-source/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
        run(f'clang++ {LINKED_LIBS} 4ed.o -o 4ed')

        if full_rebuild:
            print('Run verification script')
            run(f'{FCODER_ROOT}/code/4coder_kv/tools/find-todo.sh')


except Exception as e:
    print(f'Error: {e}')

script_end = time.time()
print(f'-----------\nScript total time: {script_end - script_begin:.3f} seconds')