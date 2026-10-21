import json
import os
import subprocess
import glob
import re
import git
import lizard
import time

audit = {}
PATH = "src/main.c"
GIT_REPO_PATH = ".."


def generateAudit(PATH, GIT_REPO_PATH):
    global audit
    gccTest(PATH=PATH)
    checkGit(GIT_REPO_PATH=GIT_REPO_PATH)
    gitLeakes(GIT_REPO_PATH=GIT_REPO_PATH)
    checkNewDependencies(GIT_REPO_PATH=GIT_REPO_PATH)
    clangTidy(target_file=PATH)
    clangFormat(target_file=PATH)
    runLizard(target_file=PATH)
    runValgrind()
    # valgrind()
    # Magic Numbers
    # integration test
    # logic test
    return print(audit)


# Wirte into json file
def write(arg, value, clear_audit=False):
    global audit

    if clear_audit:
        audit = {}

    audit[arg] = value


def gccTest(PATH):
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
            PATH,
            "-o",
            "test",
        ],
        capture_output=True,
        text=True,
    )
    text = result.stderr
    if result.returncode != 0:
        print("Test ended with output of:", text)
        print("Compilation error. Abording request.")
        write("compiler", "not OK")
    else:
        print("compilation succesfull, continuing")
        write("compiler", f"OK {text}")


def checkGit(GIT_REPO_PATH=".."):
    try:
        repo = git.Repo(GIT_REPO_PATH)
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


def gitLeakes(GIT_REPO_PATH=".."):
    report_file = "leaks.json"
    # flag --format nahrazen za -r (report-path)
    payload = [
        "gitleaks",
        "detect",
        f"--source={GIT_REPO_PATH}",
        f"-r={report_file}",
        "-v",
    ]

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
        print("Gitleaks execution failed")
        return False
        write("gitLeakes", "NOT OK")

    # Načetly se leaky ze souboru
    write("gitLeakes", "NOT OK")
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


def checkNewDependencies(GIT_REPO_PATH=".."):
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
        repo = git.Repo(GIT_REPO_PATH)
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
            "Unauthorized dependencies",
        )
        return False
    else:
        print("Dependencies OK")
        write("checkNewDependencies", "OK")
        return True


def clangTidy(target_file="../src/main.c"):
    checks = ",".join(
        [
            "clang-analyzer-*",
            "bugprone-*",
            "-bugprone-easily-swappable-parameters",
            "-bugprone-narrowing-conversions",
            "-bugprone-implicit-widening-of-multiplication-result",
            "-bugprone-suspicious-enum-usage",
            "-bugprone-unused-raii",
            "-cert-err58-cpp",
        ]
    )
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

    has_warnings = "warning:" in output or "error:" in output

    if has_warnings:
        print(f"Clang-Tidy found issues:\n{output}")
        # Extract the first few lines of warnings for the log
        summary = "\n".join(output.split("\n")[:3])
        write("runClangTidy", "NOT OK")
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
            "runClangFormat",
            "Code style violation. NOT OK",
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
        print(f"High cyclomatic complexity detected in: {', '.join(complex_functions)}")
        write(
            "runLizard",
            f"Too complex functions, NOT OK",
        )
        return False
    else:
        print("Code complexity is well within limits")
        write("runLizard", "OK")
        return True


def runValgrind(target_binary="./test"):
    if not os.path.exists(target_binary):
        write("valgrind", "Binary file not found.")
        return False

    # Příkaz spustí program pod Valgrindem a zkontroluje memory leaky
    command = [
        "valgrind",
        "--leak-check=full",
        "--error-exitcode=1",  # Pokud najde leak, vrattí návratový kód 1
        target_binary,
    ]

    try:
        # Pozor: tvůj program čeká na getchar(), tak mu pošleme 'q' nebo EOF přes input
        result = subprocess.run(
            command,
            input="5\n5\nq\n",
            capture_output=True,
            text=True,
            timeout=5,
        )
    except FileNotFoundError:
        print("Valgrind is not installed.")
        write("valgrind", "Valgrind executable not found.")
        return False
    except subprocess.TimeoutExpired:
        print("Valgrind timed out (program waited for input too long).")
        write("valgrind", "Timeout - program stuck in loop.")
        return False

    if result.returncode != 0:
        print("Valgrind detected Memory Leaks!")
        print(result.stderr)  # Zde Valgrind vypíše, kde jsi zapomněl free()
        write("valgrind", "NOT OK - Memory leak detected!")
        return False
    else:
        print("Valgrind passed: No memory leaks!")
        write("valgrind", "OK")
        return True


if __name__ == "__main__":
    generateAudit(PATH, GIT_REPO_PATH)
