@echo off

:: todo(kv): setup path
call %HOME%/msvc/setup.bat

set location=%cd%
set me="%~dp0"
cd %me%
cd ..
set src_root=%cd%
mkdir ..\build
cd ..\build
set build_root=%cd%
set bin_root=%src_root%\bin
set custom_root=%src_root%\custom
set custom_bin=%custom_root\bin
cd %location%

set mode=%1
if "%mode%" == "" (set mode="-DDEV_BUILD")

set opts=""
set opts=%opts% -I%src_root% -I%custom_root% -I %src_root%/4coder_kv/libs
set opts=%opts% -Wno-everything
set opts=%opts% %mode%

set FirstError=0
pushd %build_root%
call clang++ %opts% -L kernel32.lib %bin_root%\4ed_build.cpp -o build.exe
if %ERRORLEVEL% neq 0 (set FirstError=1)
if %ERRORLEVEL% neq 0 (goto END)
popd

%build_root%\build.exe
:END
if %ERRORLEVEL% neq 0 (set FirstError=1)
