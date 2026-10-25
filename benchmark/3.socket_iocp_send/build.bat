@echo off
rem usage : build.bat [the root of a CGDK tree with its Release libs (default : ..\..)] [exe name (default : bench_windows_io)]
rem         the headers are taken from the same tree. run it in a "x64 Native Tools" prompt or let it call vcvars64 of VS 18
setlocal
set ROOT=%~1
if "%ROOT%"=="" set ROOT=%~dp0..\..
set NAME=%~2
if "%NAME%"=="" set NAME=bench_windows_io
where cl >nul 2>&1 || call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "%~dp0"
cl /nologo /O2 /EHsc /std:c++20 /permissive- /DX64 /DNDEBUG /D_CONSOLE /MD /bigobj /I "%ROOT%\include" bench_windows_io.cpp /Fe:%NAME%.exe /link /LIBPATH:"%ROOT%\lib" advapi32.lib
endlocal
