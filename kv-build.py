#!/usr/bin/env python3 -u

import os
import pathlib
import subprocess
import time
import shutil
import argparse
from enum import Enum

parser = argparse.ArgumentParser(prog='Autodraw build script')
parser.add_argument('-a', '--action', type=str, default="build")
parser.add_argument('--file',    type=str, default="")
parser.add_argument('--full',    action="store_true")
parser.add_argument('--release', action="store_true")
#
args = parser.parse_args()
if args.release:
    args.full = True
run_only     = args.action == 'run'
hotload_game = "game.cpp" in args.file  # @build_filename_hack

# NOTE: Default configuration (don't overwrite me!) #########################
default_build_level = 0
#
ed_build_level    = 0
game_build_level  = 0
meta_build_level  = 1
imgui_build_level = 1
#
asan_on = 0
COMPILE_GAME_WITH_MSVC = 0
TRACE_COMPILE_TIME     = 0
FORCE_INLINE_ON = 1
FRAMEWORK_OPTIMIZE_ON = 0
AD_PROFILE = 0
KV_SLOW    = 0
STOP_DEBUGGING_BEFORE_BUILD = 1  #NOTE(kv) uncheck when you wanna debug the reload itself



# NOTE: Override configuration ############################
#asan_on          = 1
#ed_build_level   = 1
#meta_build_level = 0
STOP_DEBUGGING_BEFORE_BUILD = 0


# Your config end ############################

if asan_on:
    COMPILE_GAME_WITH_MSVC = 1

pjoin = os.path.join

def meets_level(level):
    global build_level
    return build_level >= level

DEBUG_MODE = 0 if args.release else 1
build_level = default_build_level
build_level = args.full
SHIP_MODE = 1-DEBUG_MODE
if SHIP_MODE:
    asan_on = 0

HOME = os.path.expanduser("~")
OUTDIR=pjoin(HOME, '4coder')
FCODER_ROOT=pjoin(HOME, '4ed')
CODE=pjoin(FCODER_ROOT, "code")
NON_SOURCE=pjoin(FCODER_ROOT, "4coder-non-source")
CODE_KV = pjoin(CODE, '4coder_kv')
OS_WINDOWS = int(os.name== "nt")
OS_MAC = int(not OS_WINDOWS)

DOT_DLL=".dll" if OS_WINDOWS else ".so"
DOT_EXE='.exe' if OS_WINDOWS else ''
DOT_OBJ='.obj' if OS_WINDOWS else '.o'
remedybg = f"remedybg.exe"
CPP_VERSION      = "-std:c++20"

warning_list = [
    #"-Werror", "-Wextra",
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
CLANG_WARNINGS = ' '.join(warning_list)


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

script_failed = False

def run(command, exit_on_failure=True, update_env={}):
    printed_command = True
    if printed_command:
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
        # NOTE: run success
        end = time.time()
        if (end - begin) > 0.05:
            if not printed_command:
                print(command)
            print(f'Time taken: {end - begin:.3f} seconds')
    else:
        # NOTE: Run fail
        if not printed_command:
            print(command)
        global script_failed
        script_failed = True
        if exit_on_failure:
            exit(1)

    return process

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

def check_extension(extensions, filename):
    for extension in extensions:
        if filename.endswith(extension):
            return True
    return False

def list_all_cpp_files_top_level(root_path):
    extensions = [".cpp", ".c"]
    result = []
    for (dirpath, dirnames, filenames) in os.walk(root_path):
        cpp_files = [f for f in filenames
                     if check_extension(extensions, f)]
        result.extend(cpp_files)
        break
    return result

def list_all_files(root_path):
    result = []
    for (dirpath, dirnames, filenames) in os.walk(root_path):
        result.extend(filenames)
    return result

def get_file_stem(path):
    return pathlib.Path(path).stem

def space_join(list):
    return " ".join(list)


script_begin = time.time()

sanitize_address = "-fsanitize=address" if asan_on else ""

class Compiler(Enum):
    ClangCl = 1
    MSVC    = 2

# NOTE: parameters are just suggestions
def run_compiler(compiler, input_files, output_file, debug_mode,
                 compiler_flags="", linker_flags="",
                 compile_only=False, link_only=False,
                 no_ccache=False, exit_on_failure=True, no_warnings=False):
    global asan_on
    # NOTE: if asan is on then you have to use msvc
    if asan_on:
        compiler = Compiler.MSVC

    is_clang = compiler == Compiler.ClangCl
    is_msvc  = compiler == Compiler.MSVC

    assert(is_clang or is_msvc)

    if asan_on and not debug_mode:
        assert(False and "asan has to be in debug mode")

    compiler_exe = "clang-cl" if is_clang else "cl"

    debug_flag = ""
    if debug_mode:
        debug_flag = "-Zi -Ob1"  # NOTE "-Zi" means that you produce a separate pdb fie
    compiler_flags += f" {debug_flag}"

    optimization = "" if debug_mode else "-O2"
    compiler_flags += f" {optimization}"

    maybe_ccache = "ccache"
    if no_ccache or (not compile_only) or asan_on:
        maybe_ccache = ""

    if output_file != "":
        if compile_only:
            if is_msvc:
                compiler_flags += f" -Fo: {output_file}"
            else: # NOTE: probably a clang-cl bug that you can't use the same syntax?
                compiler_flags += f" -o {output_file}"
        else:
            linker_flags += f" -OUT:{output_file}"

    asan_flag = "-fsanitize=address" if asan_on else ""

    if not compile_only:
        if debug_mode:
            linker_flags += " -DEBUG"

    if asan_on:
        linker_flags += " /INCREMENTAL:NO"

    if compile_only:
        linker_flags = ""
    elif linker_flags != "":
        linker_flags = "-link" + linker_flags

    if is_clang:
        compiler_flags += f" {CPP_VERSION} -D_CRT_SECURE_NO_WARNINGS -FC"
    if is_msvc:
        compiler_flags += f" {CPP_VERSION} -Zc:strictStrings- -D_CRT_SECURE_NO_WARNINGS -FC"

    if is_msvc:
        warnings = " -wd4200 -wd4201 -wd4100 -wd4101 -wd4189 -wd4815 -wd4505 -wd4701 -wd4816 -wd4702"
    if is_clang:
        warnings = CLANG_WARNINGS
    if not no_warnings:
        compiler_flags += f" -W4 -WX {warnings}"

    command = f"\
{maybe_ccache} {compiler_exe} \
{"-c" if compile_only else ""} {input_files}    \
{asan_flag}  \
{"" if link_only else compiler_flags} \
{linker_flags} {"-nologo" if is_msvc else ""}   \
"
    run(command, exit_on_failure)

def autogen():
    global asan_on
    old_asan_on = asan_on
    asan_on = False
    CUSTOM=f'{FCODER_ROOT}/code/custom'
    BUILD_DIR = pjoin(CUSTOM, "build")
    rm_rf(BUILD_DIR)
    mkdir_p(BUILD_DIR)
    with pushd(BUILD_DIR):
        INCLUDES=f'-I{CUSTOM} -I{CODE}'
        SYMBOLS=f'-DOS_MAC={int(OS_MAC)} -DOS_WINDOWS={int(OS_WINDOWS)} -DOS_LINUX=0 -DKV_INTERNAL={DEBUG_MODE} -DKV_SLOW={KV_SLOW}'
        compiler_flags=f"{SYMBOLS} {INCLUDES}"
        
        if meets_level(meta_build_level):
            print('4coder API parser/generator')
            run_compiler(Compiler.ClangCl, f"{CODE}/meta_main.cpp", "ad_meta.exe",
                         debug_mode=True, compiler_flags=compiler_flags)
            input_files = " ".join([f"{CODE}/game",
                                    f"{CODE}/4ed_api_implementation.cpp",
                                    f"{CODE}/platform_win32/win32_4ed_functions.cpp",
                                    f"{CODE}/custom/4coder_token.cpp",
                                    f"{CODE}/4coder_game_shared.h",
                                    f"{CODE}/4ed_render_target.cpp",
                                   ])
            run(f"ad_meta.exe {input_files}")

            print('Lexer: Generate (one-time thing)')
            LEXER_GEN = f"lexer_gen{DOT_EXE}"
            run_compiler(Compiler.ClangCl, pjoin(CODE_KV, '4coder_kv_skm_lexer_gen.cpp'), LEXER_GEN, 
                         debug_mode=True, compiler_flags=compiler_flags)
            #
            print('running lexer generator')
            mkdir_p(f'{CODE_KV}/generated')
            run(f'{LEXER_GEN} {CODE_KV}/generated')
        
            meta_macros="-DMETA_PASS"
            print('preproc_file: Generate')
            preproc_file=pjoin(BUILD_DIR, "4coder_command_metadata.i")
            run(f'clang++ {meta_macros} {compiler_flags} "{CODE_KV}/4coder_kv.cpp" -E -o {preproc_file}')
            
            print('Meta-generator')
            run_compiler(Compiler.ClangCl, f"{CUSTOM}/4coder_metadata_generator.cpp", f"metadata_generator{DOT_EXE}",
                         debug_mode=True, compiler_flags=compiler_flags)
            run(f'metadata_generator -R "{CUSTOM}" {preproc_file}')

    asan_on = old_asan_on

def build_game():
    try:  # NOTE: Compiling the game
        with open("game_dll.lock", "w") as file:
            file.write("this is a lock file\n")
        print(f'========Producing game{DOT_DLL}========')
        DOT_LIB=".lib"
        GAME_MAIN = pjoin(CODE, "game", "game_main.cpp")
        GAME_CPP  = pjoin(CODE, "game", "game.cpp")
        if TRACE_COMPILE_TIME:
            # NOTE: ftime-trace only works with "-c"
            run(f'clang++ -c {pjoin(CODE, "game", "game.cpp")}  {INCLUDES} {SYMBOLS} -Od -ftime-trace')
        else:
            cl_or_clang_cl = Compiler.MSVC if COMPILE_GAME_WITH_MSVC else Compiler.ClangCl
            # NOTE: "Zc:strictStrings-" must be put after the cpp version for some reason
            #
            MSVC_COMPILE_FLAGS = f"{INCLUDES} {SYMBOLS}"
            if not hotload_game:
                # NOTE: Compile the framework
                run_compiler(cl_or_clang_cl, GAME_MAIN, "",
                             debug_mode=(not FRAMEWORK_OPTIMIZE_ON),
                             compiler_flags=f"{INCLUDES} {SYMBOLS}",
                             compile_only=True)
            # NOTE: Compile the driver and the framework
            run_compiler(cl_or_clang_cl, f'{GAME_CPP} game_main.obj {space_join(imgui_object_files)}', f"game{DOT_DLL}",
                         compiler_flags=f"{INCLUDES} {SYMBOLS}",
                         linker_flags="-DLL -export:game_api_export", debug_mode=True)

    finally:
        os.remove("game_dll.lock")

try:
    if asan_on:
        print("[asan_on]")

    os.chdir(f'{OUTDIR}')
    print(f'Workdir: {os.getcwd()}')

    if run_only:
        if OS_WINDOWS:
            run(f"{remedybg} start-debugging")
        else:
            run(pjoin(OUTDIR, f'4ed{DOT_EXE}'))
    else:
        #-NOTE(kv): Compile the project---------------------------

        # NOTE(kv): remedy stop debugging
        if OS_WINDOWS and STOP_DEBUGGING_BEFORE_BUILD:
            run(f"{remedybg} stop-debugging")

        INCLUDES=f'-I{CODE} -I{CODE}/libs -I{CODE}/libs/imgui -I{CODE}/custom -I{NON_SOURCE}/foreign/freetype2 -I{CODE}/4coder_kv'
        #
        COMMON_SYMBOLS=f"-DFRED_SUPER -DFTECH_64_BIT -DSHIP_MODE={1-DEBUG_MODE}"
        SYMBOLS=f"-DKV_SLOW={KV_SLOW} -DAD_PROFILE={AD_PROFILE} -DKV_INTERNAL={DEBUG_MODE} -DFRED_INTERNAL -DDO_CRAZY_EXPENSIVE_ASSERTS {COMMON_SYMBOLS}" if DEBUG_MODE else COMMON_SYMBOLS

        BINARY_NAME = "4ed" if DEBUG_MODE else "4ed_stable"

        imgui_dir = f"{CODE}/libs/imgui"
        imgui_cpp_basenames = list_all_cpp_files_top_level(imgui_dir)
        imgui_object_files  = [f"{get_file_stem(file)}{DOT_OBJ}" 
                               for file in imgui_cpp_basenames]

        if (not hotload_game) and (not TRACE_COMPILE_TIME):
            # NOTE(kv): cleanup build dir (TODO: arrange our build output directory so we don't have to do manual cleaning crap)
            if build_level >= 1:
                delete_all_pdb_files(OUTDIR)

            autogen()

            # NOTE: Editor build
            if OS_WINDOWS:
                PLATFORM_CPP = f"{CODE}/platform_win32/win32_4ed.cpp"
                WINDOWS_LIBS = "user32.lib winmm.lib gdi32.lib comdlg32.lib userenv.lib"
                FREETYPE_LIB = f"{NON_SOURCE}/foreign/x64/freetype.lib"
                LINKED_LIBS=f"{FREETYPE_LIB} {WINDOWS_LIBS} opengl32.lib {NON_SOURCE}/res/icon.res"
            else:
                PLATFORM_CPP = f"{CODE}/platform_mac/mac_4ed.mm"
                LINKED_LIBS=f"{NON_SOURCE}/foreign/x64/libfreetype-mac.a -framework Cocoa -framework QuartzCore -framework CoreServices -framework OpenGL -framework IOKit -framework Metal -framework MetalKit"
            # NOTE Add "-fms-runtime-lib=dll_dbg" to call with debug crt

            # NOTE Compiling DearImgui
            imgui_files = [pjoin(imgui_dir, f) for f in imgui_cpp_basenames]
            imgui_backend_files = [f"{imgui_dir}/backends/imgui_impl_win32.cpp", f"{imgui_dir}/backends/imgui_impl_opengl3.cpp"]
            imgui_config = '-DIMGUI_USER_CONFIG=\\"ad_imgui_config.h\\"'
            if meets_level(imgui_build_level):
                for file in (imgui_files + imgui_backend_files):
                    # NOTE(kv): Since we run the build through the shell, we gotta escape the double-quotes :>
                    run_compiler(Compiler.ClangCl, file, "",
                                 debug_mode=DEBUG_MODE,
                                 compiler_flags=f"{imgui_config} -I{imgui_dir} -I{CODE}",
                                 compile_only=True, no_warnings=True)

            if meets_level(ed_build_level):
                print('========Producing 4ed========')
                ed_obj = f"{BINARY_NAME}{DOT_OBJ}"
                run_compiler(Compiler.ClangCl, PLATFORM_CPP,
                             ed_obj, debug_mode=DEBUG_MODE,
                             compiler_flags=f"{INCLUDES} {SYMBOLS}",
                             compile_only=True)
    
                # NOTE(kv): Linking 4coder
                USE_DEBUG_CRT = "-Xlinker -nodefaultlib:libcmt -Xlinker -defaultlib:libcmtd" if DEBUG_MODE else ""
                imgui_backend_object_files = [f"imgui_impl_win32{DOT_OBJ}", f"imgui_impl_opengl3{DOT_OBJ}"]
                run_compiler(Compiler.ClangCl,
                             f"{ed_obj} {space_join(imgui_object_files)} {space_join(imgui_backend_object_files)}",
                             f"{BINARY_NAME}{DOT_EXE}", debug_mode=DEBUG_MODE,
                             linker_flags=f"{LINKED_LIBS}",
                             link_only=True,
                             exit_on_failure=False)

            # NOTE: shipping shaders
            if SHIP_MODE:
                OPENGL_OUTDIR = pjoin(OUTDIR, "opengl")
                mkdir_p(OPENGL_OUTDIR)
                for filename in ["vertex_shader.glsl", "geometry_shader.glsl", "fragment_shader.glsl"]:
                    shutil.copy(pjoin(CODE, "opengl", filename), pjoin(OPENGL_OUTDIR, filename))
        if meets_level(game_build_level):
            build_game()

        if SHIP_MODE:
            print("NOTE: Setup symlinks, because my life just is complicated like that!")
            symlink_force(pjoin(CODE_KV, "config.4coder"),   pjoin(OUTDIR, "config.4coder"))
            symlink_force(pjoin(CODE_KV, "theme-kv.4coder"), pjoin(OUTDIR, 'themes', "theme-kv.4coder"))
            symlink_force(pjoin(CODE, "project.4coder"),     pjoin(OUTDIR, "project.4coder"))
            symlink_force(pjoin(CODE, "data"),               pjoin(OUTDIR, "data"))

except Exception as e:
    print(f'Error: {e}')
    exit(1)

if script_failed:
    print(f"Script failed due to silent failure in the middle")
    exit(1)

script_end = time.time()
print(f'-----------\nScript total time: {script_end - script_begin:.3f} seconds')
