#!/usr/bin/python3
import re
import os

os.chdir("../code")
files = [f for f in os.listdir('.') if os.path.isfile(f)]
print(files)

for name in ["intrinsics", "math", 'scalar']:
    from0 = "kv" + '_' + name + ".h"
    to = "kv" + '-' + name + '.h'

    try:
        os.rename(from0, to)
    except:
        pass

    for filename in files:
        print(filename)
        with open(filename, 'r') as f:
            lines = f.readlines()
        temp = filename+'-tmp'
        with open(temp, 'w') as f:
            for line in lines:
                f.write(re.sub(from0, to, line))
            os.rename(temp, filename)
