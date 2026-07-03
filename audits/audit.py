import json
import os
import subprocess
import glob
import re
import git
import lizard
audit = {}

def generateAudit():
    global audit
    test()
    logs()
    checkGit()
    gitLeakes()
    checkNewDependencies()
    clangTidy()
    clangFormat()
    runLizard()
    print(audit)


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
    result = subprocess.run(
        [
            "gcc",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Wshadow",
            "-Wconversion",
            "-Werror",
            "../src/main.c",
            "-o",
            "test",
        ],
        capture_output=True,
        text=True,
    )
    text = result.stdout
    if result.returncode != 0:
        print("Test ended with output of:", text)
        print("Compilation error. Abording request.")
        write("compiler", text)
    else:
        print("compilation succesfull, continuing")
        write("compiler", f"OK {text}")

def logs():
    if os.path.exists("test"):
        os.chmod("test", 0o755)
        subprocess.run(["./test"], capture_output=True)

    log_output = ""
    log_files = glob.glob("../logs/*.log")

    for file in log_files:
        with open(file, "r", encoding="utf-8") as f:
            log_output += f.read()

    log_output = log_output.strip()

    if log_output != "":
        print("Logs are not empty:\n", log_output)
        write("logs", f"logs are not empty after production use: {log_output}")
    else:
        print("Logs are good")
        write("Logs", "OK")

import git


def checkGit():
    try:
        repo = git.Repo("..")
    except git.InvalidGitRepositoryError:
        print("Not a git repository")
        write("Git", "Fail to open repo")
        return False

    commits = list(repo.iter_commits())
    if not commits:
        write("Git", "OK, no commits")
        return True

    target_author = commits[0].author.name
    suspicious_found = False

    for commit in commits:
        if commit.author.name != target_author:
            break

        commit_message = commit.message.strip()
        if len(commit_message) < 3:
            write("Git", f"Suspicious git message: '{commit_message}'")
            suspicious_found = True

    if suspicious_found:
        print("Check git naming of commits")
        write(
            "Git",
            "Suspicious git message found for current user, but not fatal",
        )
    else:
        write("Git", "OK")
        
def gitLeakes():
    report_file = "leaks.json"
    # flag --format nahrazen za -r (report-path)
    payload = ["gitleaks", "detect", "--source=..", f"-r={report_file}", "-v"]

    try:
        result = subprocess.run(payload, capture_output=True, text=True)
    except FileNotFoundError:
        print("Gitleaks not installed.")
        write("gitLeakes", "Gitleaks executable not found.")
        return False

    # Gitleaks vrací 0 pokud nic nenašel
    if result.returncode == 0:
        print("Gitleaks passed: No secrets found.")
        write("gitLeakes", "OK")
        # Pro jistotu smažeme starý report, pokud by existoval
        if os.path.exists(report_file):
            os.remove(report_file)
        return True

    # Pokud vrátil jiný kód než 0, buď našel leaky, nebo selhal
    if not os.path.exists(report_file):
        print(f"Gitleaks execution failed: {result.stderr.strip()}")
        write("gitLeakes", f"Gitleaks error: {result.stderr.strip()[:50]}")
        return False

    # Načetly se leaky ze souboru
    write("gitLeakes", "NOT OK, CRITICAL, ENV OR OTHER PUBLISHED")
    try:
        with open(report_file, "r", encoding="utf-8") as f:
            found = json.load(f)

        for leak in found:
            print(f"--- found ---")
            print(f"file: {leak.get('File')}")
            print(f"Line:  {leak.get('StartLine')}")
            print(f"Commit: {leak.get('Commit')}")
            print(f"type:    {leak.get('Description')}")

    except json.JSONDecodeError:
        print("Error while parsing Gitleaks report file.")
        write("gitLeakes", "Error while parsing JSON.")
    finally:
        # Po analýze dočasný soubor s leaky smažeme
        if os.path.exists(report_file):
            os.remove(report_file)

    return False

def checkNewDependencies():
    valid_dependencies = {
        "stdio.h",
        "stdlib.h",
        "string.h",
        "stdbool.h",
        "ctype.h",
        "unistd.h",
        "assert.h",
    }

    try:
        repo = git.Repo("..")
    except git.InvalidGitRepositoryError:
        print("Not a git repository")
        write("checkNewDependencies", "Git repository not found.")
        return False

    commits = list(repo.iter_commits(max_count=50))
    if not commits:
        print("No commits found")
        write("checkNewDependencies", "No commits found.")
        return False

    current_commit = commits[0]
    current_author = current_commit.author.email
    base_commit = None

    for commit in commits[1:]:
        if commit.author.email != current_author:
            base_commit = commit
            break

    if not base_commit:
        if current_commit.parents:
            base_commit = current_commit.parents[0]
        else:
            print("First commit in repository")
            write("checkNewDependencies", "First commit.")
            return True

    diffs = base_commit.diff(current_commit, create_patch=True)
    new_deps = set()
    include_regex = re.compile(r'^\+\s*#include\s*[<"]([^>"]+)[>"]')

    for d in diffs:
        if d.a_path and d.a_path.endswith((".c", ".h")):
            diff_text = d.diff.decode("utf-8", errors="ignore").split("\n")
            for line in diff_text:
                if line.startswith("+"):
                    match = include_regex.match(line)
                    if match:
                        new_deps.add(match.group(1))

    unavailable = new_deps - valid_dependencies
    unavailable = {k for k in unavailable if not k.startswith("src/")}

    if unavailable:
        print(f"Unauthorized dependencies found: {unavailable}")
        write(
            "checkNewDependencies",
            f"Unauthorized dependencies: {', '.join(unavailable)}",
        )
        return False
    else:
        print("Dependencies OK")
        write("checkNewDependencies", "OK")
        return True

def clangTidy(target_file="../src/main.c"):
    checks = "bugprone-*,clang-analyzer-*,cert-*"

    command = [
        "clang-tidy",
        target_file,
        f"-checks={checks}",
        "--",
        "-Wall",
        "-Wextra",
    ]

    try:
        result = subprocess.run(command, capture_output=True, text=True)
    except FileNotFoundError:
        print("Clang-Tidy is not installed on this system")
        write("runClangTidy", "Clang-Tidy executable not found.")
        return False

    output = result.stdout.strip()
    errors = result.stderr.strip()

    has_warnings = "warning:" in output or "error:" in output

    if has_warnings:
        print(f"Clang-Tidy found issues:\n{output}")
        # Extract the first few lines of warnings for the log
        summary = "\n".join(output.split("\n")[:3])
        write("runClangTidy", f"Issues found: {summary}")
        return False
    else:
        print("Clang-Tidy check passed successfully")
        write("runClangTidy", "OK")
        return True

def clangFormat(target_file="../src/main.c"):
    command = [
        "clang-format",
        "--dry-run",
        "-Werror",
        "--style=LLVM",
        target_file,
    ]

    try:
        result = subprocess.run(command, capture_output=True, text=True)
    except FileNotFoundError:
        print("Clang-Format is not installed on this system")
        write("runClangFormat", "Clang-Format executable not found.")
        return False

    if result.returncode != 0:
        print("Clang-Format detected code style violations!")
        write(
            "runClangFormat", "Code style violation. Please run clang-format -i --style=LLVM src/main.c."
        )
        return False
    else:
        print("Code formatting is perfect")
        write("runClangFormat", "OK")
        return True

def runLizard(target_file="../src/main.c", max_complexity=15):
    try:
        analysis = lizard.analyze_file(target_file)
    except Exception as e:
        print(f"Lizard analysis failed: {e}")
        write("runLizard", "Lizard analysis failed.")
        return False

    complex_functions = []

    for func in analysis.function_list:
        if func.cyclomatic_complexity > max_complexity:
            complex_functions.append(
                f"{func.name} (CCN: {func.cyclomatic_complexity}, Lines: {func.length})"
            )

    if complex_functions:
        print(
            f"High cyclomatic complexity detected in: {', '.join(complex_functions)}"
        )
        write(
            "runLizard",
            f"Too complex functions found: {', '.join(complex_functions)}",
        )
        return False
    else:
        print("Code complexity is well within limits")
        write("runLizard", "OK")
        return True

generateAudit()