import os
import platform

path = os.getcwd()

files = os.listdir(path + "/src")

files = ["src/" + file for file in files if file[-2:] == ".c" or file[-2:] == ".h"]

compiler = "clang -g -I include"

command = (
    compiler
    + (" -o jacv" if platform.system() != "Windows" else " -o jacv.exe ")
    + " ".join(files)
)
os.system(command)
