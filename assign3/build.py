import os
import argparse
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-i", type=str, default="assign3-test0_29")
parser.add_argument("-o", type=str, default="build")
args = parser.parse_args()
in_dir = args.i
out_dir = args.o

for file in os.listdir(in_dir):
    in_file = os.path.join(in_dir, file)
    out_file = os.path.join(out_dir, file.strip(".c"))
    cmd = "/usr/local/llvm-10.0.1/bin/clang -c -g3 -O0 -emit-llvm " + in_file + " -o " + out_file + ".bc"
    res = subprocess.run(cmd, shell=True, text=True, capture_output=True)
    return_code = res.returncode
    if return_code != 0:
        sys.stderr.write("cmd: %s Error!\n" % cmd)   
    cmd = "/usr/local/llvm-10.0.1/bin/llvm-dis " + out_file + ".bc" +" -o " + out_file + ".ll"    
    res = subprocess.run(cmd, shell=True, text=True, capture_output=True)
    return_code = res.returncode
    if return_code != 0:
        sys.stderr.write("cmd: %s Error!\n" % cmd)  