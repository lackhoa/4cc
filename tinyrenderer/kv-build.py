import os
import time
import subprocess

pjoin = os.path.join

OS_WINDOWS = int(os.name== "nt")
HOME = os.path.expanduser("~")
ROOT = pjoin(HOME, "4ed", "code", "tinyrenderer")

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

os.chdir(ROOT)
run(f'clang++ main.cpp')
