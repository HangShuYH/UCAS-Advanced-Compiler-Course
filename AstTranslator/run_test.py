import argparse
import os
import sys
import subprocess

def copyCFile(from_file: str, to_file: str):
    INCLUDE_str = """#include <stdio.h>\n#include <stdlib.h>\n"""
    GET_str = """int GET() {int a;scanf("%d", &a); return a;}\n"""
    PRINT_str = """void PRINT(int a) {fprintf(stderr,"%d", a);}\n"""
    MALLOC_str = """void* MALLOC(int a) {return (void*)malloc(a);}\n"""
    FREE_str = """void FREE(void * a) {free(a);}\n"""
    to_lines = list()
    to_lines.append(INCLUDE_str)
    to_lines.append(GET_str)
    to_lines.append(PRINT_str)
    to_lines.append(MALLOC_str)
    to_lines.append(FREE_str)
    with open(from_file, "r") as from_f:
        from_lines = from_f.readlines()
        for from_line in from_lines:
            if from_line.find("extern") == 0:
                continue
            to_lines.append(from_line)

    with open(to_file, "w") as to_f:
        to_f.writelines(to_lines)

def get_std_result(file: str) -> str:
    binary = file + ".out"
    compile_cmd = "gcc %s -o %s" % (file, binary)
    compile_result = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True)
    return_code = compile_result.returncode
    if return_code != 0:
        sys.stderr.write("cmd: %s Error!\nReturn with Code %d\n" % (compile_cmd, return_code))
        sys.exit(-1)
    
    exec_result = subprocess.run(binary, shell=True, capture_output=True, text=True)
    stderr_output = exec_result.stderr
    return stderr_output

def get_interpreter_result(file: str, interpreter: str) -> str:
    interpreter_cmd = """%s "$(cat %s)" """ % (interpreter, file)
    interpreter_result = subprocess.run(interpreter_cmd, shell=True, capture_output=True, text=True)
    return_code = interpreter_result.returncode
    if return_code != 0:
        sys.stderr.write("cmd: %s Error!\nReturn with Code %s\n" % (interpreter_cmd, return_code))
        sys.exit(-1)
    
    stderr_output = interpreter_result.stderr
    return stderr_output

parser = argparse.ArgumentParser()
parser.add_argument("-i", type=str, default="tests")
parser.add_argument("-o", type=str, default="tests-std-c")
parser.add_argument("-interp", type=str, default="build/ast-interpreter")
args = parser.parse_args()
test_dir = os.path.abspath(args.i)
test_std_c_dir = os.path.join(test_dir, args.o)
interp = os.path.abspath(args.interp)
if not os.path.exists(test_std_c_dir):
    os.mkdir(test_std_c_dir)
elif not os.path.isdir(test_std_c_dir):
    sys.stderr.write("%s Not a Directory!\n" % test_std_c_dir)
    sys.exit(-1)

passed = True
for file_name in os.listdir(test_dir):
    file = os.path.join(test_dir, file_name)
    if not file.endswith(".c"):
        continue
    file_std_c = os.path.join(test_std_c_dir, file_name)
    copyCFile(file, file_std_c)
    
    std_c_result = get_std_result(file_std_c)
    interpreter_result = get_interpreter_result(file, interp)

    if std_c_result == interpreter_result:
        print("\033[32mTest Passed: %s\033[0m" % file)
    else:
        print("\033[31mTest Failed: %s\033[0m" % file)
        print("\033[31mstd_c_result: %s\033[0m" % std_c_result)
        print("\033[31minterpreter_result: %s\033[0m" % interpreter_result)
        passed = False
    print("-" * 50)
if passed:
    print("\033[32mAll Tests Passed!\033[0m")
else:
    print("\033[32mSome Tests Failed!\033[0m")