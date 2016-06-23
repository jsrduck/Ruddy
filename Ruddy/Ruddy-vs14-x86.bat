@if "%OVERBOSE%" == "" echo off
@setlocal

CALL "%VS140COMNTOOLS%VsDevCmd.bat"

CALL "Ruddy.exe" %1

CALL "%LLVM_BINARY_PATH%\Release\bin\llc.exe" -filetype=obj %%~n1.ll

CALL link %%~n1.obj ..\WindowsImpl\Debug\IO.obj ..\WindowsImpl\Debug\stdafx.obj /subsystem:console /OUT:%%~n1.exe
