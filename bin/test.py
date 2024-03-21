#!/usr/bin/python3

import re
import sys
import subprocess
import os

dir_path = os.path.dirname(os.path.realpath(__file__))

cwd = os.getcwd()

testfile = sys.argv[1]

pattern = re.compile("// expect:(.*)")

expects = []

for i, line in enumerate(open(testfile, 'r', encoding='utf-8', errors='ignore')):
    for match in re.finditer(pattern, line):
        expects.append(match.group(1).strip())

bin = "moser"
if (os.name == 'nt' ) : 
    bin = "moser.exe"
    moc = dir_path + os.sep +".." + os.sep + "out" + os.sep + "build" + os.sep + "x64-Release" + os.sep + bin
else :
    moc = dir_path + os.sep +".." + os.sep + "build" + os.sep + bin

cmd = moc + " " + testfile

p = subprocess.Popen( cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

(output, err) = p.communicate()
 
p_status = p.wait()

out = output.decode(errors="ignore").strip()
if( out  == ""):
    out = err.decode(errors="ignore").strip()

if(out == ""):
    if(len(expects) == 0) :
        print ("ok")
        exit(0)
        
    print ("not ok [" + str(len(expects)) + "]" )
    exit(1)

asserts = out.split(os.linesep)

if len(asserts) != len(expects):
    print("mismatch of expects and asserts\n")
    print("expects: " + str(len(expects)))
    print("asserts: " + str(len(asserts)))
    print("'" + out + "'")
    print("'" + err.decode(errors="ignore").strip() + "'")
    exit(1)

for i in range(0,len(expects)):
    if ( asserts[i].strip() != expects[i]):
        print( asserts[i] + " does not match " + expects[i] )
        exit(1)

print ("ok [" + str(len(expects)) + "]" )
