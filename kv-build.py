#!/usr/bin/env python3 -u

# NOTE(kv): 4ed build script
# NOTE(kv): assumes only Windows and Mac (and clang)

import os
import subprocess
import sys
import time

DEBUG_MODE = True
HOME = os.path.expanduser("~")
FCODER_USER=f'{HOME}/4coder'  # NOTE: for debug build
FCODER_ROOT=f'{HOME}/4ed'
SOURCE=f"{FCODER_ROOT}/code"
NON_SOURCE=f"{FCODER_ROOT}/4coder-non-source"
FCODER_KV = f'{SOURCE}/4coder_kv'
OS_WINDOWS = os.name== "nt"
OS_MAC = not OS_WINDOWS

# todo(kv): we should change to a build  dir instead, so we can clean all the crap out!
def delete_all_pdb_files(dir_name):
    files = os.listdir(dir_name)
    for item in files:
        if item.endswith(".pdb") or item.endswith('.ilk'):
            os.remove(os.path.join(dir_name, item))

# NOTE: unused
def setup_msvc_envvars():
    if not OS_WINDOWS:
        return

    ROOT = f"{HOME}/msvc"
    MSVC_VERSION = "14.38.33130"
    MSVC_HOST        = "Hostx64"
    MSVC_ARCH       = "x64"
    SDK_VERSION    = "10.0.22621.0"
    SDK_ARCH          = "x64"

    os.environ['ROOT'] = ROOT
    os.environ['MSVC_VERSION'] = MSVC_VERSION
    os.environ['MSVC_HOST']  = MSVC_HOST
    os.environ['MSVC_ARCH'] = MSVC_ARCH
    os.environ['SDK_VERSION'] = SDK_VERSION
    os.environ['SDK_ARCH']      = SDK_ARCH

    MSVC_ROOT   = f"{ROOT}/VC/Tools/MSVC/{MSVC_VERSION}"
    SDK_INCLUDE = f"{ROOT}/Windows Kits/10/Include/{SDK_VERSION}"
    SDK_LIBS         = f"{ROOT}/Windows Kits/10/Lib/{SDK_VERSION}"

    os.environ['MSVC_ROOT'] = MSVC_ROOT
    os.environ['SDK_INCLUDE'] = SDK_INCLUDE
    os.environ['SDK_LIBS'] = SDK_LIBS

    os.environ["VCToolsInstallDir"] = f"{MSVC_ROOT}/"
    PATH = os.environ["PATH"]
    os.environ["PATH"] = f"{MSVC_ROOT}/bin/{MSVC_HOST}/{MSVC_ARCH};{ROOT}/Windows Kits/10/bin/{SDK_VERSION}/{SDK_ARCH};{ROOT}/Windows Kits/10/bin/{SDK_VERSION}/{SDK_ARCH}/ucrt;{PATH}"
    os.environ["INCLUDE"] = f"{MSVC_ROOT}/include;{SDK_INCLUDE}/ucrt;{SDK_INCLUDE}/shared;{SDK_INCLUDE}/um;{SDK_INCLUDE}/winrt;{SDK_INCLUDE}/cppwinrt"
    os.environ["LIB"]          = f"{MSVC_ROOT}/lib/{MSVC_ARCH};{SDK_LIBS}/ucrt/{SDK_ARCH};{SDK_LIBS}/um/{SDK_ARCH}"

def run(command, update_env={}):
    if OS_WINDOWS:
        command = f'{HOME}/msvc/setup.bat && {command}'

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
    SYMBOLS=f'-DOS_MAC={int(OS_MAC)} -DOS_WINDOWS={int(OS_WINDOWS)} -DOS_LINUX=0 -DKV_INTERNAL={int(DEBUG_MODE)} -DKV_SLOW={int(DEBUG_MODE)}'
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

    arch = "-m64"
    debug="-g" if DEBUG_MODE else ""

    if run_only:
        run(f'{FCODER_USER}/4ed')
    else:
        if full_rebuild:  # do some generation business in the custom layer
            autogen()

        print(f'NOTE: Building the 4ed binary at {os.getcwd()}')

        DLL_EXTENSION="dll" if OS_WINDOWS else "so"
        print('Producing 4ed_app.{DLL_EXTENSION}')
        delete_all_pdb_files(OUTDIR)
        # TODO(kv): vet these warnings
        WARNINGS="-Wno-write-strings -Wno-null-dereference -Wno-comment -Wno-switch -Wno-missing-declarations -Wno-logical-op-parentheses -Wno-deprecated-declarations -Wno-tautological-compare -Wno-unused-result -Wno-nullability-completeness"
        INCLUDES=f'-I{SOURCE} -I{SOURCE}/custom -I{NON_SOURCE}/foreign/freetype2 -I{SOURCE}/4coder_kv -I{SOURCE}/4coder_kv/libs'
        #
        COMMON_SYMBOLS="-DFRED_SUPER -DFTECH_64_BIT"
        SYMBOLS=f"-DKV_SLOW=1 -DKV_INTERNAL=1 -DFRED_INTERNAL -DDO_CRAZY_EXPENSIVE_ASSERTS {COMMON_SYMBOLS}" if DEBUG_MODE else COMMON_SYMBOLS
        #
        OPTIMIZATION_LEVEL="-O0" if DEBUG_MODE else "-O3"
        COMPILE_FLAGS=f"{WARNINGS} {INCLUDES} {SYMBOLS} {OPTIMIZATION_LEVEL} {debug} -m64 -std=c++11"
        run(f'ccache clang++ {COMPILE_FLAGS} -c {SOURCE}/4ed_app_target.cpp -o 4ed_app.o')
        #
        run(f'clang++ -shared 4ed_app.o -o 4ed_app.{DLL_EXTENSION} -Wl,-export:app_get_functions {debug}')

        print('Producing 4ed')
        if OS_WINDOWS:
            PLATFORM_CPP = f"{SOURCE}/platform_win32/win32_4ed.cpp"
        else:
            PLATFORM_CPP =  f"{SOURCE}/platform_mac/mac_4ed.mm"
         #
        run(f'ccache clang++ {COMPILE_FLAGS} -I{SOURCE}/platform_all -c {PLATFORM_CPP} -o 4ed.o')
        #
        if OS_WINDOWS:
            LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/freetype.lib -luser32.lib -lwinmm.lib -lgdi32.lib -lopengl32.lib -lcomdlg32.lib -luserenv.lib {NON_SOURCE}/res/icon.res"
        else:
            LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
        #
        run(f'clang++ {LINKED_LIBS} 4ed.o -o 4ed.exe {debug}')

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
