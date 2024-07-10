#!/usr/bin/env python3 -u

# NOTE: configuration #########################
COMPILE_GAME_WITH_MSVC = 1
TRACE_COMPILE_TIME     = 0
DISABLE_GAME_DEBUG_INFO     = 0  # NOTE(kv): Not worth disabling with MSVC
if TRACE_COMPILE_TIME:
    DISABLE_GAME_DEBUG_INFO = 1
MSVC_FRAMEWORK_OPTIMIZATION = "-O2"
MSVC_GAME_OPTIMIZATION = "-Od"  # -Ob2 helps a bit, but isn't worth the slow compile time
AD_PROFILE = 1
KV_SLOW = 0
STOP_DEBUGGING_BEFORE_BUILD = 0
GAME_OPTIMIZATION = "-O0"  # NOTE: this is clang, which we don't use anymore

import os
import subprocess
import time
import shutil
import argparse


parser = argparse.ArgumentParser(prog='Autodraw build script')
parser.add_argument('-a', '--action', type=str, default="build")
parser.add_argument('--file', type=str, default="")
parser.add_argument('--full', action="store_true")
parser.add_argument('--release', action="store_true")
args = parser.parse_args()
run_only         = args.action == 'run'
hotload_game     = "game.cpp" in args.file  # @build_filename_hack

# NOTE(kv): 4ed build script
# NOTE(kv): assumes only Windows and Mac (and clang)

pjoin = os.path.join


DEBUG_MODE = not args.release
EDITOR_OPTIMIZATION = "-O0" if DEBUG_MODE else "-O3"  # NOTE: Tried -O2 and even -O1, it's still slow af
SHIP_MODE = 1-DEBUG_MODE

HOME = os.path.expanduser("~")
OUT_DEBUG=pjoin(HOME, '4coder')  # NOTE: for debug build
FCODER_ROOT=pjoin(HOME, '4ed')
CODE=pjoin(FCODER_ROOT, "code")
NON_SOURCE=pjoin(FCODER_ROOT, "4coder-non-source")
CODE_KV = pjoin(CODE, '4coder_kv')
OS_WINDOWS = int(os.name== "nt")
OS_MAC = int(not OS_WINDOWS)

DOT_DLL=".dll" if OS_WINDOWS else ".so"
DOT_EXE='.exe' if OS_WINDOWS else ''
remedybg = "remedybg.exe"
CPP_VERSION      = "-std=c++20"
MSVC_CPP_VERSION = "/std:c++20"
if OS_WINDOWS:
    if TRACE_COMPILE_TIME:
        CALL_CL = f'{CODE}/call_vcvarsall.bat && cl'  # NOTE: slow!
    else:
        CALL_CL = f'{HOME}/msvc/setup.bat && cl'

WARNINGS_ARRAY = [
    "-Werror", 
    "-Wall",
    "-Wextra",
    "-Wimplicit-int-float-conversion",
    "-Wshadow",

    "-Wno-unused-variable",
    "-Wno-unused-but-set-variable",
    "-Wno-write-strings",
    "-Wno-null-dereference",
    "-Wno-comment",
    "-Wno-switch",
    "-Wno-missing-declarations",
    "-Wno-deprecated-declarations",
    "-Wno-missing-braces",  # todo(kv): removeme
    "-Wno-unused-parameter",
    "-Wno-unused-function",
    "-Wno-backslash-newline-escape",
    "-Wno-deprecated-enum-enum-conversion",
    "-Wno-deprecated-anon-enum-enum-conversion",
]
WARNINGS = ' '.join(WARNINGS_ARRAY)

def mkdir_p(path):
    os.makedirs(path, exist_ok=True)

# todo(kv): We should change to a build dir instead, so we can clean all the crap out!
def delete_all_pdb_files(dir_name):
    files = os.listdir(dir_name)
    for item in files:
        if item.endswith(".pdb") or item.endswith('.ilk') or item.endswith(".dSYM"):
            os.remove(os.path.join(dir_name, item))

def rm_tree(path, topdown=False) :
    for root, dirs, files in os.walk(path):
        for file in files:
            file_path = os.path.join(path, file)
            os.remove(file_path)
        for dir in dirs:
            dir_path = os.path.join(path, dir)
            os.rmdir(dir_path)
    os.removedirs(path)

def rm_rf_inner(path):
    if os.path.islink(path):
        os.unlink(path)
    elif os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path)
    elif os.path.exists(path):
        print('idk know what this path is')
        exit(1)

def rm_rf(*paths):
    for path in paths:
        rm_rf_inner(path)

def symlink_force(src, dst):
    rm_rf(dst)
    mkdir_p( os.path.dirname(dst) )
    os.symlink(src, dst)

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

class pushd: # pylint: disable=invalid-name
    __slots__ = ('_pushstack',)

    def __init__(self, dirname):
        self._pushstack = list()
        self.pushd(dirname)

    def __enter__(self):
        return self

    def __exit__(self, exec_type, exec_val, exc_tb) -> bool:
        # skip all the intermediate directories, just go back to the original one.
        if self._pushstack:
            os.chdir( self._pushstack.pop(0) )
        if exec_type:
            return False
        return True

    def popd(self) -> None:
        if len(self._pushstack):
            os.chdir(self._pushstack.pop())

    def pushd(self, dirname) -> None:
        self._pushstack.append(os.getcwd())
        os.chdir(dirname)

def autogen():
    CUSTOM=f'{FCODER_ROOT}/code/custom'
    BUILD_DIR = pjoin(CUSTOM, "build")
    rm_rf(BUILD_DIR)
    mkdir_p(BUILD_DIR)
    with pushd(BUILD_DIR):
        # print(f"BUILD_DIR: {os.getcwd()}")
        INCLUDES=f'-I{CUSTOM} -I{CODE}'
        OPTIMIZATION='-O0'
        SYMBOLS=f'-DOS_MAC={int(OS_MAC)} -DOS_WINDOWS={int(OS_WINDOWS)} -DOS_LINUX=0 -DKV_INTERNAL={DEBUG_MODE} -DKV_SLOW={KV_SLOW}'
        arch="-m64"
        opts=f"{WARNINGS} {SYMBOLS} {debug_flag} {INCLUDES} {OPTIMIZATION}"
        preproc_file=pjoin(BUILD_DIR, "4coder_command_metadata.i")
    
        print('Lexer: Generate (one-time thing)')
        LEXER_GEN = pjoin(BUILD_DIR, f"lexer_gen{DOT_EXE}")
        run(f'clang++ {pjoin(CODE_KV, '4coder_kv_skm_lexer_gen.cpp')} {arch} {opts} {debug_flag} {WARNINGS} {CPP_VERSION} {OPTIMIZATION} -o {LEXER_GEN}')
        #
        print('running lexer generator')
        mkdir_p(f'{CODE_KV}/generated')
        run(f'{LEXER_GEN} {CODE_KV}/generated')
    
        meta_macros="-DMETA_PASS"
        print('preproc_file: Generate')
        run(f'clang++ -I{CUSTOM} {meta_macros} {arch} {opts} {debug_flag} {CPP_VERSION} "{CODE_KV}/4coder_kv.cpp" -E -o {preproc_file}')
        #
        print('Meta-generator: Compile & Link')
        run(f'ccache clang++ -c "{CUSTOM}/4coder_metadata_generator.cpp" -I"{CUSTOM}" {opts} {CPP_VERSION} -o "{BUILD_DIR}/metadata_generator.o"')
        #
        run(f'clang++ -I"{CUSTOM}" "{CUSTOM}/metadata_generator.o" -o "{BUILD_DIR}/metadata_generator{DOT_EXE}" -g')
        #
        print('Meta-generator: Run')
        run(f'"{BUILD_DIR}/metadata_generator" -R "{CUSTOM}" {preproc_file}')

script_begin = time.time()

try:
    OUTDIR = OUT_DEBUG

    os.chdir(f'{OUTDIR}')
    print(f'Workdir: {os.getcwd()}')

    if DEBUG_MODE:
        full_rebuild = args.full
    else:
        full_rebuild = True

    arch = "-m64"
    debug_flag="-g" if DEBUG_MODE else ""
    if DISABLE_GAME_DEBUG_INFO:
        debug_flag=""  # NOTE: -g0 is NOT how you do this in clang

    if run_only:
        if OS_WINDOWS:
            run(f"{remedybg} start-debugging")
        else:
            run(pjoin(OUT_DEBUG, f'4ed{DOT_EXE}'))
    else:
        # NOTE(kv): remedy stop debugging
        if OS_WINDOWS and STOP_DEBUGGING_BEFORE_BUILD:
            run(f"{remedybg} stop-debugging")

        INCLUDES=f'-I{CODE}/libs -I{CODE} -I{CODE}/custom -I{NON_SOURCE}/foreign/freetype2 -I{CODE}/4coder_kv -I{CODE}/4coder_kv/libs'
        #
        COMMON_SYMBOLS=f"-DFRED_SUPER -DFTECH_64_BIT -DSHIP_MODE={1-DEBUG_MODE}"
        SYMBOLS=f"-DKV_SLOW={KV_SLOW} -DAD_PROFILE={AD_PROFILE} -DKV_INTERNAL=1 -DFRED_INTERNAL -DDO_CRAZY_EXPENSIVE_ASSERTS {COMMON_SYMBOLS}" if DEBUG_MODE else COMMON_SYMBOLS
        #
        COMPILE_FLAGS=f"{WARNINGS} {INCLUDES} {SYMBOLS} {EDITOR_OPTIMIZATION} {debug_flag} -m64 {CPP_VERSION}"

        BINARY_NAME = "4ed" if DEBUG_MODE else "4ed_stable"
        if (not hotload_game) and (not TRACE_COMPILE_TIME):
            # NOTE(kv): cleanup build dir (TODO: arrange our build output directory so we don't have to do manual cleaning crap)
            delete_all_pdb_files(OUTDIR)

            if full_rebuild:  # do some generation business in the custom layer
                autogen()

            print('========Producing 4ed========')
            if OS_WINDOWS:
                PLATFORM_CPP = f"{CODE}/platform_win32/win32_4ed.cpp"
                WINDOWS_LIBS = "-luser32.lib -lwinmm.lib -lgdi32.lib -lcomdlg32.lib -luserenv.lib"
                FREETYPE_LIB = f"{NON_SOURCE}/foreign/x64/freetype.lib"
                LINKED_LIBS=f"{FREETYPE_LIB} {WINDOWS_LIBS} -lopengl32.lib {NON_SOURCE}/res/icon.res"
            else:
                PLATFORM_CPP = f"{CODE}/platform_mac/mac_4ed.mm"
                LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
            #
            # NOTE: static_dbg equals "MTd (multi-threaded debug)"
            # NOTE Add "-fms-runtime-lib=static_dbg" to call with debug crt
            run(f'ccache clang++ -c {PLATFORM_CPP} -o {BINARY_NAME}.o {INCLUDES} {COMPILE_FLAGS}')
            # NOTE: clang uses the non-debug CRT and then fails when linking with something that uses the debug CRT: https://stackoverflow.com/questions/41850296/link-dynamic-c-runtime-with-clang-windows
            USE_DEBUG_CRT = "-Xlinker -nodefaultlib:libcmt -Xlinker -defaultlib:libcmtd" if DEBUG_MODE else ""
            run(f'clang++ {BINARY_NAME}.o {LINKED_LIBS} -o {BINARY_NAME}{DOT_EXE} {debug_flag}')
            # NOTE: shipping shaders
            if SHIP_MODE:
                OPENGL_OUTDIR = pjoin(OUTDIR, "opengl")
                mkdir_p(OPENGL_OUTDIR)
                for filename in ["vertex_shader.glsl", "geometry_shader.glsl", "fragment_shader.glsl"]:
                    shutil.copy(pjoin(CODE, "opengl", filename), pjoin(OPENGL_OUTDIR, filename))
        try:  # NOTE: Compiling the game
            with open("game_dll.lock", "w") as file:
                file.write("this is a lock file\n")
            print(f'========Producing game{DOT_DLL}========')
            DOT_LIB=".lib"
            GAME_MAIN = pjoin(CODE, "game", "game_main.cpp")
            GAME_CPP  = pjoin(CODE, "game", "game.cpp")
            if COMPILE_GAME_WITH_MSVC:
                # NOTE: "Zc:strictStrings-" must be put after the cpp version for some reason
                MSVC_WARNINGS = "-wd4200 -wd4201 -wd4100 -wd4101 -wd4189 -wd4815 -wd4505 -wd4701 -wd4816 -wd4702"
                MSVC_COMPILE_FLAGS = f"-W4 -WX {MSVC_WARNINGS} -FC {"" if DISABLE_GAME_DEBUG_INFO else "-Z7"} {INCLUDES} {SYMBOLS} -D_CRT_SECURE_NO_WARNINGS {MSVC_CPP_VERSION} -Zc:strictStrings- -nologo"
                #
                if not hotload_game:
                    run(f'{CALL_CL} -c {GAME_MAIN} {MSVC_COMPILE_FLAGS} {MSVC_FRAMEWORK_OPTIMIZATION}')
                run(f'{CALL_CL} -LD {GAME_CPP} game_main.obj {MSVC_COMPILE_FLAGS} {MSVC_GAME_OPTIMIZATION} -link {"-DEBUG:NONE" if DISABLE_GAME_DEBUG_INFO else "-DEBUG"} -OUT:game{DOT_DLL} -export:game_api_export -nologo')
            else: # NOTE: Compile with clang
                if TRACE_COMPILE_TIME:
                    # NOTE: ftime-trace only works with "-c"
                    run(f'clang++ -c {pjoin(CODE, "game", "game.cpp")}  {COMPILE_FLAGS} {GAME_OPTIMIZATION} -ftime-trace')
                else:
                    run(f'clang++ -shared {GAME_MAIN} {GAME_CPP} -o game{DOT_DLL} {COMPILE_FLAGS} {GAME_OPTIMIZATION} -Wl,-export:game_api_export')
        finally:
            os.remove("game_dll.lock")

        if full_rebuild:
            print("NOTE: Setup symlinks, because my life just is complicated like that!")
            for outdir in [OUT_DEBUG]:
                symlink_force(pjoin(CODE_KV, "config.4coder"),   pjoin(outdir, "config.4coder"))
                symlink_force(pjoin(CODE_KV, "theme-kv.4coder"), pjoin(outdir, 'themes', "theme-kv.4coder"))
                symlink_force(pjoin(CODE, "project.4coder"),  pjoin(outdir, "project.4coder"))
                symlink_force(pjoin(CODE, "data"), pjoin(outdir, "data"))

except Exception as e:
    print(f'Error: {e}')
    exit(1)

script_end = time.time()
print(f'-----------\nScript total time: {script_end - script_begin:.3f} seconds')
