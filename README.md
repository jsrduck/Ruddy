# Ruddy
A strong, statically-typed programming language

Prereqs:

1. Install Python version 2.7 (not version 3) from https://www.python.org/
2. Download Quex from http://quex.sourceforge.net/ (currently using Quex version 0.65.10)
3. Make sure you have QUEX_PATH environment variable pointing to the location where you installed Quex
4. Install and build the LLVM suite from http://llvm.org/releases/download.html. You'll have to follow the directions on how to build
from http://llvm.org/docs/, as, unfortunately, there are no prebuilt binaries available at the moment. It has it's own dependencies.
Ruddy is currently supported on llvm 3.8.0
5. Create an environment variable LLVM_INCLUDE_PATH that includes the location of the includes directories for LLVM files. It should
look something like: "C:\llvm-3.8.0.src\include;C:\llvm-3.8.0.binaries\include" and LLVM_BINARY_PATH for the directory where your
binaries were created, ie "C:\llvm-3.8.0.binaries"
