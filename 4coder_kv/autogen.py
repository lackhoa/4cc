#!/usr/bin/env python3 -u

# todo(kv): this file really has to go away!

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
    else:
        exit(1)

    print()
    return process

def mtime(path):
    try:
        return os.path.getmtime(path)
    except:
        return 0

# cd to script directory
HOME=os.path.expanduser("~")
FCODER_USER=f'{HOME}/4coder'
FCODER_ROOT=f'{HOME}/4ed'
CUSTOM=f'{FCODER_ROOT}/code/custom'
FCODER_KV = f'{FCODER_ROOT}/code/4coder_kv'
SOURCE=f'{FCODER_KV}/4coder_kv.cpp'
DEBUG_MODE = True
DEBUG_MODE_01 = 1 if DEBUG_MODE else 0

try:
    os.chdir(f'{FCODER_USER}')
    print(f'Workdir: {os.getcwd()}')

    run_only       = (len(sys.argv) > 1 and sys.argv[1] == 'run')

    ADDRESS_SANITIZER_ON = False

    INCLUDES=f'-I{CUSTOM} -I{FCODER_KV}/libs'
    OPTIMIZATION='-O0' if DEBUG_MODE else '-O2'
    SYMBOLS=f'-DOS_MAC=1 -DOS_WINDOWS=0 -DOS_LINUX=0 -DKV_INTERNAL={DEBUG_MODE_01} -DKV_SLOW={DEBUG_MODE_01}'
    arch="-m64"
    debug="-g" if DEBUG_MODE else ""
    opts=f"-Wno-write-strings -Wno-null-dereference -Wno-comment -Wno-switch -Wno-missing-declarations -Wno-logical-op-parentheses {SYMBOLS} {debug} {INCLUDES} {OPTIMIZATION}"
    preproc_file="4coder_command_metadata.i"

    if run_only:
        dyld_insert_libraries="DYLD_INSERT_LIBRARIES=/usr/local/Cellar/llvm/17.0.6/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" if ADDRESS_SANITIZER_ON else ""
        command = f'{dyld_insert_libraries} {FCODER_USER}/4ed > /dev/null'
        # Does emacs output pipe slow down 4coder? Very possible!
        run(command) # Run in a new process group

    else:
        if full_rebuild:  # Lexer generator
            print('Lexer: Generate (one-time thing)')
            OPTIMIZATION="-O0"
            run(f'clang++ {FCODER_KV}/4coder_kv_skm_lexer_gen.cpp {arch} {opts} {debug} -Wno-tautological-compare -std=c++11 {OPTIMIZATION} -o {FCODER_KV}/lexer_generator')
            #
            print('running lexer generator')
            run(f'mkdir -p {FCODER_KV}/generated && {FCODER_KV}/lexer_generator {FCODER_KV}/generated')
            run(f'rm {FCODER_KV}/lexer_generator & rm -rf {FCODER_KV}/lexer_generator.dSYM')

        sanitize_address = '-fsanitize=address' if ADDRESS_SANITIZER_ON else ''
        meta_macros="-DMETA_PASS"
        if full_rebuild:
            print('preproc_file: Generate')
            run(f'clang++ -I{CUSTOM} {meta_macros} {arch} {opts} {debug} -std=c++11 "{SOURCE}" -E -o {preproc_file}')
            #
            print('Meta-generator: Compile & Link')
            run(f'ccache clang++ -c "{CUSTOM}/4coder_metadata_generator.cpp" -I"{CUSTOM}" {opts} -O2 -std=c++11 -o "{CUSTOM}/metadata_generator.o"')
            #
            run(f'clang++ -I"{CUSTOM}" "{CUSTOM}/metadata_generator.o" -o "{CUSTOM}/metadata_generator"')
            #
            print('Meta-generator: Run')
            run(f'"{CUSTOM}/metadata_generator" -R "{CUSTOM}" "{os.getcwd()}/{preproc_file}"')

        print("NOTE: Setup 4coder config files")
        run(f'ln -sf "{FCODER_KV}/config.4coder" "{FCODER_USER}/config.4coder"')
        run(f'ln -sf "{FCODER_KV}/theme-kv.4coder" "{FCODER_USER}/themes/theme-kv.4coder"')

except Exception as e:
    print(f'Error: {e}')

finally:
    run(f'rm -f "{CUSTOM}/metadata_generator.o" "{CUSTOM}/metadata_generator" {preproc_file}')
