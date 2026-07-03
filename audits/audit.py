import json
import os
import subprocess
def generateAudit():
    inputs()
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

GLOBAL_GIT_USERNAME = None

# Wirte into json file
def writeInJson(arg, value, filename=None, clearFile=False):
    global GLOBAL_GIT_USERNAME
    if not filename:
        filename = f"auditsFiles/{GLOBAL_GIT_USERNAME}.audit"
        
    if not clearFile and os.path.exists(filename) and os.path.getsize(filename) > 0:
        with open(filename, "r", encoding="utf-8") as file:
            try:
                data = json.load(file)
            except json.JSONDecodeError:
                data = {}
    else:
        data = {}

    data[arg] = value

    with open(filename, "w", encoding="utf-8") as file:
        json.dump(data, file, indent=4, ensure_ascii=False)

# Safe input
def inp(ques, poss=[]):
    if poss != []:
        print(f"Possible answer: {*poss}", sep='\n')
    validAns = False
    while not validAns:
        ans = input(ques)
        if poss != []:
            if ans in poss:
                validAns = True
        else:
            validAns = True
    return validAns

#input git username
def inputs():
    global GLOBAL_GIT_USERNAME
    nameOfc, nameNorm = None
    try:
        nameOfc = subprocess.check_output(["gh", "auth", "status"], stderr=subprocess.STDOUT).decode("utf-8")
    except:
        pass
    try: 
        nameNorm = subprocess.check_output(["git", "config", "user.name"]).decode("utf-8").strip()
    except:
        pass
    knowName = False
    if nameNorm:
        ans = inp(f"Is your username on gh {nameNorm}", ["yes", "no"])
        if ans == "yes":
            name = ans
            knowName = True
    if nameOfc and not knowName:
        ans = inp(f"Is your username {nameOfc}", ["yes", "no"])
        if ans == "yes":
            name = ans
            knowName = True
    
    
    if (knowName and name == None) or (not knowName and name != None):
        raise ValueError("knowName is not validing name")
    
    if not knowName:
        name = inp("Whats your git username?")

    GLOBAL_GIT_USERNAME = name
    
# tests
def test():
    c_source_file = "game.c"
    binary_output = "./test"
    compilator = inp("What is you compiler?", ["gcc", "clang", "msvc", "other"])
    if compilator == "other":
        compilator = inp("What is your compilator? \n NOTE: If you are using some other compilator, you are more than welcomed to contribute into our test() function in audit.py")
    structOfTest = {
        "typeOfCompilator": compilator,
        "warnings": None,
        "valid": False
    }
    
    match compilator:
        case "gcc":
            gccTest()
        case "clang": 
            clangTest()
        case "msvc":
            msvcTest()
        case "other":
            pass
    subprocess.run()