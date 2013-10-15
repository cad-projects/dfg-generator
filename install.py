#!/usr/bin/python
import sys
import os
import shutil
import fileinput
import argparse

def replaceAll(file,searchExp,replaceExp):
   for line in fileinput.input(file, inplace=1):
      if searchExp in line:
         line = line.replace(searchExp,replaceExp)
      sys.stdout.write(line)

def printError(errorMsg):
   print ""
   print "[ERROR]: " + errorMsg
   print ""
   sys.exit()

build_dir = "_build_"

script_pathname = os.path.dirname(sys.argv[0]) 
cadlib_path = os.path.abspath(script_pathname + "/lib")

parser = argparse.ArgumentParser(description='Available options')
parser.add_argument('-prefix', action="store", default='/opt/compiler', help='Installation directory')
parser.add_argument('-compile-llvm', dest="compile_llvm", action="store_true", default=False, help='Enable the compilation of LLVM sources (default=False)')
results = parser.parse_args()

print ""
print ".:: Current configuration ::."
print " - Installation directory = "+str(results.prefix)
print "-----------------------------"

if os.path.isdir(build_dir) == False:
   os.makedirs(build_dir)
   os.makedirs(build_dir + "/sources")
os.chdir(build_dir)
#starting the configuration of the framework
current_path = os.getcwd()

llvm_build = os.path.abspath("build-llvm")
if (results.compile_llvm):
   print "\n.:: Configuring LLVM ... ::."
   if os.path.exists("sources/clang-3.2.src.tar.gz") == False:
      os.system("wget http://llvm.org/releases/3.2/clang-3.2.src.tar.gz -O sources/clang-3.2.src.tar.gz");
   if os.path.exists("sources/llvm-3.2.src.tar.gz") == False:
      os.system("wget http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz -O sources/llvm-3.2.src.tar.gz");
   if os.path.exists("sources/compiler-rt-3.2.src.tar.gz") == False:
      os.system("wget http://llvm.org/releases/3.2/compiler-rt-3.2.src.tar.gz -O sources/compiler-rt-3.2.src.tar.gz");
   if os.path.exists("sources/polly-3.2.src.tar.gz") == False:
      os.system("wget http://llvm.org/releases/3.2/polly-3.2.src.tar.gz -O sources/polly-3.2.src.tar.gz");
   if os.path.isdir("sources/llvm-3.2.src") == False:
      os.system("tar xvf sources/llvm-3.2.src.tar.gz -C sources")
      os.system("tar xvf sources/clang-3.2.src.tar.gz -C sources")
      os.system("mv sources/clang-3.2.src sources/llvm-3.2.src/tools/clang")
      os.system("tar xvf sources/compiler-rt-3.2.src.tar.gz -C sources")
      os.system("mv sources/compiler-rt-3.2.src sources/llvm-3.2.src/projects/compiler-rt")
      os.system("tar xvf sources/polly-3.2.src.tar.gz -C sources")
      os.system("mv sources/polly-3.2.src sources/llvm-3.2.src/tools/polly")
   llvm_src = os.path.abspath("sources/llvm-3.2.src")
   if os.path.isdir("build-llvm") == False:
      os.makedirs("build-llvm")
      os.chdir("build-llvm")
      os.system("cmake -DCMAKE_INSTALL_PREFIX=" + results.prefix + " " + llvm_src)
      os.system("make")
      os.system("make install")
      os.chdir(current_path)

print "\n.:: Configuring LLVM plugin ... ::."
##cmake-based tools
if os.path.isdir(cadlib_path) == False:
   printError('LLVM plugin NOT found')
if os.path.isdir("build-plugin") == False:
   os.makedirs("build-plugin")
   os.chdir("build-plugin")
   os.system("cmake "+cadlib_path+" -DCMAKE_INSTALL_PREFIX="+results.prefix + " -DLLVM_INSTALL_ROOT=" + llvm_build)
   os.chdir(current_path)
#compilation and installation
os.chdir("build-plugin")
if (os.system("make") != 0):
   printError("Problems during LLVM plugin compilation")
os.system("make install")
os.chdir(current_path)

print "\n.:: Creating configuration script ... ::."
conf_script = open(results.prefix+"/setup.sh", 'w')
conf_script.write("#!/bin/sh\n")
conf_script.write("export PATH="+results.prefix+"/bin:$PATH\n")
conf_script.write("export LD_LIBRARY_PATH="+results.prefix+"/lib:$LD_LIBRARY_PATH\n")
conf_script.close()
os.chmod(results.prefix+"/setup.sh", 0775)

print ""
print "Installation completed with SUCCESS!"
print ""

