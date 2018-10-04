# Ruddy
A strong, statically-typed programming language

Prereqs:

1. Install Python version 2.7 (not version 3) from https://www.python.org/
2. Download Quex from http://quex.sourceforge.net/ (currently using Quex version 0.65.10)
3. Make sure you have QUEX_PATH environment variable pointing to the location where you installed Quex
4. Install the LLVM suite via vcpkg (https://github.com/Microsoft/vcpkg). Built using version 6.0.0-1.
5. Create an environment variable LLVM_BINARY_PATH that includes the location of the llvm tools. Example path:
D:\GitHub\vcpkg\packages\llvm_x86-windows\tools\llvm
6. Install the boost-property-tree package via vcpkg
7. Because of a limitation in Visual Studio, you may have to run "build" 4 times the fisrt time (there are 4 custom build steps that VS won't run in sequence)
