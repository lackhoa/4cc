#!/usr/bin/env python3 -u
import subprocess
import os

pjoin = os.path.join
HOME        = os.path.expanduser("~")
FCODER_ROOT = pjoin(HOME, '4ed')
CODE        = pjoin(FCODER_ROOT, "code")

command = f"{pjoin(CODE, "kv-build.py")} game"
print (command)
process = subprocess.run(command, shell=True, capture_output=True )
if len(process.stdout):
    print(process.stdout.decode("utf-8"))
if len(process.stderr):
    print(process.stderr.decode("utf-8"))
exit(process.returncode)
