#!/usr/bin/python3

import re
import sys
import subprocess
import os
from pathlib import Path

cwd = os.getcwd()
mydir = os.path.dirname(os.path.realpath(__file__))

tests = []

for path in Path(cwd).rglob('*.msr'):
    tests.append(path.relative_to(cwd).as_posix())

for test in tests:

    cmd = "python3 " + mydir + os.sep + "test.py " + test
    print (cmd)
    p = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

    (output, err) = p.communicate()
 
    p_status = p.wait()

    if(p_status != 0) : 
        print("moc crushed " + str(p_status))
        exit(1)

    out = output.decode(errors="ignore").strip()

    if( out  == ""):
        out = err.decode(errors="ignore").strip()

    if(out == ""):
        print ("not ok output empty")
        exit(1)

    if(out[0:2] != "ok") :        
        print ("not ok!" + out[0:2])
        exit(1)

print("ok all tests successful.")