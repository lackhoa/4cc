#!/usr/bin/env python3 -u


import os
import subprocess
import sys
import time
import shutil

# NOTE(kv): 4ed build script
# NOTE(kv): assumes only Windows and Mac (and clang)

pjoin = os.path.join

# NOTE: configuration #########################
DEBUG_MODE = 0 if (len(sys.argv) == 2 and sys.argv[1] == 'release') else 1
FORCE_FULL_REBUILD = 1
STOP_DEBUGGING_BEFORE_BUILD = 0

HOME = os.path.expanduser("~")
FCODER_USER=pjoin(HOME, '4coder')  # NOTE: for debug build
FCODER_ROOT=pjoin(HOME, '4ed')
CODE=pjoin(FCODER_ROOT, "code")
NON_SOURCE=pjoin(FCODER_ROOT, "4coder-non-source")
FCODER_KV = pjoin(CODE, '4coder_kv')
OS_WINDOWS = int(os.name== "nt")
OS_MAC = int(not OS_WINDOWS)

DOT_DLL=".dll" if OS_WINDOWS else ".so"
DOT_EXE='.exe' if OS_WINDOWS else ''
remedybg = "remedybg.exe"

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
    if OS_WINDOWS:
        command = f'{HOME}/msvc/setup.bat && {command}'

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
        SYMBOLS=f'-DOS_MAC={int(OS_MAC)} -DOS_WINDOWS={int(OS_WINDOWS)} -DOS_LINUX=0 -DKV_INTERNAL={DEBUG_MODE} -DKV_SLOW={DEBUG_MODE}'
        arch="-m64"
        opts=f"{WARNINGS} {SYMBOLS} {debug} {INCLUDES} {OPTIMIZATION} -g"
        preproc_file=pjoin(BUILD_DIR, "4coder_command_metadata.i")
    
        print('Lexer: Generate (one-time thing)')
        LEXER_GEN = pjoin(BUILD_DIR, f"lexer_gen{DOT_EXE}")
        run(f'clang++ {pjoin(FCODER_KV, '4coder_kv_skm_lexer_gen.cpp')} {arch} {opts} {debug} {WARNINGS} -std=c++11 {OPTIMIZATION} -o {LEXER_GEN}')
        #
        print('running lexer generator')
        mkdir_p(f'{FCODER_KV}/generated')
        run(f'{LEXER_GEN} {FCODER_KV}/generated')
    
        meta_macros="-DMETA_PASS"
        print('preproc_file: Generate')
        run(f'clang++ -I{CUSTOM} {meta_macros} {arch} {opts} {debug} -std=c++11 "{FCODER_KV}/4coder_kv.cpp" -E -o {preproc_file}')
        #
        print('Meta-generator: Compile & Link')
        run(f'ccache clang++ -c "{CUSTOM}/4coder_metadata_generator.cpp" -I"{CUSTOM}" {opts} -std=c++11 -o "{BUILD_DIR}/metadata_generator.o"')
        #
        run(f'clang++ -I"{CUSTOM}" "{CUSTOM}/metadata_generator.o" -o "{BUILD_DIR}/metadata_generator{DOT_EXE}" -g')
        #
        print('Meta-generator: Run')
        run(f'"{BUILD_DIR}/metadata_generator" -R "{CUSTOM}" {preproc_file}')

script_begin = time.time()

try:
    FCODER_STABLE = pjoin(HOME, "4coder_stable")
    OUTDIR = FCODER_USER if DEBUG_MODE else FCODER_STABLE

    os.chdir(f'{OUTDIR}')
    print(f'Workdir: {os.getcwd()}')

    run_only       = (len(sys.argv) == 2 and sys.argv[1] == 'run')
    hotload_game   = (len(sys.argv) == 2 and sys.argv[1] == 'game')
    full_rebuild   = True
    if DEBUG_MODE and (not FORCE_FULL_REBUILD):
        full_rebuild = (len(sys.argv) > 1 and sys.argv[1] == 'full')

    arch = "-m64"
    debug="-g" if DEBUG_MODE else ""

    if run_only:
        if OS_WINDOWS:
            run(f"{remedybg} start-debugging")
        else:
            run(pjoin(FCODER_USER, f'4ed{DOT_EXE}'))
    else:
        # NOTE(kv): remedy stop debugging
        if OS_WINDOWS and STOP_DEBUGGING_BEFORE_BUILD:
            run(f"{remedybg} stop-debugging")

        # NOTE(kv): cleanup build dir (TODO: arrange our build output directory so we don't have to do manual cleaning crap)
        delete_all_pdb_files(OUTDIR)

        if full_rebuild:  # do some generation business in the custom layer
            autogen()

        INCLUDES=f'-I{CODE} -I{CODE}/custom -I{NON_SOURCE}/foreign/freetype2 -I{CODE}/4coder_kv -I{CODE}/4coder_kv/libs'
        #
        COMMON_SYMBOLS=f"-DFRED_SUPER -DFTECH_64_BIT -DSHIP_MODE={1-DEBUG_MODE}"
        SYMBOLS=f"-DKV_SLOW=1 -DKV_INTERNAL=1 -DFRED_INTERNAL -DDO_CRAZY_EXPENSIVE_ASSERTS {COMMON_SYMBOLS}" if DEBUG_MODE else COMMON_SYMBOLS
        #
        OPTIMIZATION_LEVEL="-O0" if DEBUG_MODE else "-O3"  # NOTE: Tried -O2 and even -O1, it's still slow af
        COMPILE_FLAGS=f"{WARNINGS} {INCLUDES} {SYMBOLS} {OPTIMIZATION_LEVEL} {debug} -m64 -std=c++11"

        if not hotload_game:
            print('Producing 4ed')
            if OS_WINDOWS:
                PLATFORM_CPP = f"{CODE}/platform_win32/win32_4ed.cpp"
                LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/freetype.lib -luser32.lib -lwinmm.lib -lgdi32.lib -lopengl32.lib -lcomdlg32.lib -luserenv.lib {NON_SOURCE}/res/icon.res"
            else:
                PLATFORM_CPP =  f"{CODE}/platform_mac/mac_4ed.mm"
                LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
             #
            run(f'ccache clang++ -c {PLATFORM_CPP} -o 4ed.o -I{CODE}/platform_all {COMPILE_FLAGS}')
            #
            run(f'clang++ {LINKED_LIBS} 4ed.o -o 4ed{DOT_EXE} {debug}')

        print(f'Producing game{DOT_DLL}')
        DOT_LIB=".lib"
        run(f'clang++ -shared {pjoin(CODE, "game", "game.cpp")} 4ed{DOT_LIB} -o game{DOT_DLL} {COMPILE_FLAGS} -Wl,-export:game_api_export {debug}')

        if full_rebuild:
            print("NOTE: Setup symlinks, because my life just is complicated like that!")
            for outdir in [FCODER_USER, FCODER_STABLE]:
                symlink_force(pjoin(FCODER_KV, "config.4coder"),   pjoin(outdir, "config.4coder"))
                symlink_force(pjoin(FCODER_KV, "theme-kv.4coder"), pjoin(outdir, 'themes', "theme-kv.4coder"))
                symlink_force(pjoin(CODE, "project.4coder"),  pjoin(outdir, "project.4coder"))
                symlink_force(pjoin(CODE, "data"), pjoin(outdir, "data"))

except Exception as e:
    print(f'Error: {e}')
    exit(1)

script_end = time.time()
print(f'-----------\nScript total time: {script_end - script_begin:.3f} seconds')
