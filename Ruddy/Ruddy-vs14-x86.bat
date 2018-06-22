echo on
@setlocal

set PATH=%PATH%;%PROGRAMFILES(X86)%\Microsoft Visual Studio\Installer
for /f "usebackq tokens=*" %%i in (`vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\Common7\Tools\vsdevcmd.bat" (
  set "VSCMD_START_DIR=%CD%"
  CALL "%InstallDir%\Common7\Tools\vsdevcmd.bat
  CALL "Ruddy.exe" %1
  CALL "%LLVM_BINARY_PATH%\llc.exe" -filetype=obj %%~n1.ll
  CALL link %%~n1.obj ..\WindowsImpl\Debug\IO.obj ..\WindowsImpl\Debug\stdafx.obj /subsystem:console /OUT:%%~n1.exe
)
