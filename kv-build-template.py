#!/usr/bin/env python3 -u

#NOTE: Quick build template (for Windows)

import subprocess
import os
import shutil
import argparse

parser = argparse.ArgumentParser(prog='test build script')
parser.add_argument('-a', '--action', type=str, default="build")
parser.add_argument('--file', type=str, default="")

args = parser.parse_args()
run_only         = args.action == 'run'

pjoin = os.path.join

def run(command):
    print(command)
    process = subprocess.run(command, shell=True, capture_output=True)

    if len(process.stdout):
        print(process.stdout.decode("utf-8"))
    if len(process.stderr):
        print(process.stderr.decode("utf-8"))

    if process.returncode != 0:
        exit(1)

    return process

remedybg = "remedybg.exe"

#====================Build Parameters====================

FILENAME = "win32_opengl_multi"
CPP_FILENAME = f"{FILENAME}.c"
EXE_FILENAME = "{FILENAME}.exe"

#====================Actual Script====================

if run_only:
    run(f"{remedybg} start-debugging")
else:
    run(f"clang {CPP_FILENAME} -o{EXE_FILENAME}")

#===========End==============