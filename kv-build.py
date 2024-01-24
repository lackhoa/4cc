#!/usr/bin/env python3 -u

# NOTE(kv): 4ed build script

import os
import subprocess
from subprocess import PIPE, STDOUT
import sys
import time

DEBUG_MODE = False
DEBUG_MODE_01 = 1 if DEBUG_MODE else 0
HOME = os.path.expanduser("~")
FCODER_USER=f'{HOME}/4coder'  # NOTE: for debug build
FCODER_ROOT=f'{HOME}/4ed'
FCODER_KV=f'{FCODER_ROOT}/code/4coder_kv'

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

def autogen():
    CUSTOM=f'{FCODER_ROOT}/code/custom'
    INCLUDES=f'-I{CUSTOM} -I{FCODER_KV}/libs'
    OPTIMIZATION='-O0' if DEBUG_MODE else '-O2'
    SYMBOLS=f'-DOS_MAC=1 -DOS_WINDOWS=0 -DOS_LINUX=0 -DKV_INTERNAL={DEBUG_MODE_01} -DKV_SLOW={DEBUG_MODE_01}'
    arch="-m64"
    debug="-g" if DEBUG_MODE else ""
    opts=f"-Wno-write-strings -Wno-null-dereference -Wno-comment -Wno-switch -Wno-missing-declarations -Wno-logical-op-parentheses {SYMBOLS} {debug} {INCLUDES} {OPTIMIZATION}"
    preproc_file="4coder_command_metadata.i"

    print('Lexer: Generate (one-time thing)')
    OPTIMIZATION="-O0"
    run(f'clang++ {FCODER_KV}/4coder_kv_skm_lexer_gen.cpp {arch} {opts} {debug} -Wno-tautological-compare -std=c++11 {OPTIMIZATION} -o {FCODER_KV}/lexer_generator')
    #
    print('running lexer generator')
    run(f'mkdir -p {FCODER_KV}/generated && {FCODER_KV}/lexer_generator {FCODER_KV}/generated')
    run(f'rm {FCODER_KV}/lexer_generator & rm -rf {FCODER_KV}/lexer_generator.dSYM')

    meta_macros="-DMETA_PASS"
    print('preproc_file: Generate')
    run(f'clang++ -I{CUSTOM} {meta_macros} {arch} {opts} {debug} -std=c++11 "{FCODER_KV}/4coder_kv.cpp" -E -o {preproc_file}')
    #
    print('Meta-generator: Compile & Link')
    run(f'ccache clang++ -c "{CUSTOM}/4coder_metadata_generator.cpp" -I"{CUSTOM}" {opts} -O2 -std=c++11 -o "{CUSTOM}/metadata_generator.o"')
    #
    run(f'clang++ -I"{CUSTOM}" "{CUSTOM}/metadata_generator.o" -o "{CUSTOM}/metadata_generator"')
    #
    print('Meta-generator: Run')
    run(f'"{CUSTOM}/metadata_generator" -R "{CUSTOM}" "{os.getcwd()}/{preproc_file}"')

script_begin = time.time()

try:
    OUTDIR = FCODER_USER if DEBUG_MODE else f"{HOME}/4coder_stable"
    os.chdir(f'{OUTDIR}')
    print(f'Workdir: {os.getcwd()}')

    run_only       = (len(sys.argv) > 1 and sys.argv[1] == 'run')
    full_rebuild   = True  # NOTE(kv): there are commands not available in release mode
    if DEBUG_MODE:
        full_rebuild = (len(sys.argv) > 1 and sys.argv[1] == 'full')

    ADDRESS_SANITIZER_ON = False

    arch = "-m64"
    debug="-g" if DEBUG_MODE else ""

    if run_only:
        dyld_insert_libraries="DYLD_INSERT_LIBRARIES=/usr/local/Cellar/llvm/17.0.6/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" if ADDRESS_SANITIZER_ON else ""
        run(f'{dyld_insert_libraries} {FCODER_USER}/4ed > /dev/null')
    else:
        if full_rebuild:  # do some generation business in the custom layer (todo(kv): this script is busted, it's in the wrong folder!)
            autogen()

        print(f'NOTE: Building the 4ed binary at {os.getcwd()}')

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
            #
            print("NOTE: Setup 4coder config files")
            run(f'ln -sf "{FCODER_KV}/config.4coder" "{FCODER_USER}/config.4coder"')
            run(f'ln -sf "{FCODER_KV}/theme-kv.4coder" "{FCODER_USER}/themes/theme-kv.4coder"')


except Exception as e:
    print(f'Error: {e}')

script_end = time.time()
print(f'-----------\nScript total time: {script_end - script_begin:.3f} seconds')