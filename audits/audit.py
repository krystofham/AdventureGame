import json
import os
import subprocess
import glob

audit = {}
def generateAudit():
    test()
    logs()
    aiUsage()
    git()
    gitLeakes()
    checkNewDependecies()
    churnRate()
    clangTidy()
    clangFormat()
    lizard()
    pushgit()


# Wirte into json file
def write(arg, value, clear_audit=False):
    global audit

    if clear_audit:
        audit = {}

    audit[arg] = value

# tests
def test():
    c_source_file = "game.c"
    binary_output = "./test"
    gccTest()

def gccTest():
    # gcc -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror src/main.c -o test
    result = subprocess.run("gcc", "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Wconversion", "-Werror", "../src/main.c", "-o", "test", capture_output=True, text=True)
    text = result.stdout
    if result.returncode != 0:
        print("Test ended with output of:", text)
        print("Compilation error. Abording request.")
        write("compiler", text)
    else:
        print("compilation succesfull, continuing")
        write("compiler", "OK")

def logs():
    if os.path.exists("test"):
        os.chmod("test", 0o755)
        subprocess.run(["./test"], capture_output=True)

    log_output = ""
    log_files = glob.glob("../logs/*.log")

    for file in log_files:
        with open(soubor, "r", encoding="utf-8") as f:
            log_output += f.read()

    log_output = log_output.strip()

    if log_output != "":
        print("Logs are not empty:\n", log_output)
        write("logs", f"logs are not empty after production use: {log_output}")
    else:
        print("Logs are good")
        write("Logs", "OK")

