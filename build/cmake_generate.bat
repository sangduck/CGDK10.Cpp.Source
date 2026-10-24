@ECHO OFF
REM ----------------------------------------------------------------------
REM  CGDK10 for C++
REM  Generate Visual Studio projects (vcxproj + sln/slnx) from CMakeLists.txt
REM
REM  usage : cmake_generate.bat [vs18|vs17] [build]
REM          vs18  : Visual Studio 2026 -> vcxproj + .slnx  (default)
REM          vs17  : Visual Studio 2022 -> vcxproj + .sln
REM          build : also build Debug and Release after generating
REM
REM  output: build\cmake_projects\<vs18|vs17>\CGDK10.core.slnx (or .sln)
REM          (libraries are written to lib\cgdk\sdk10\windows\v<toolset>, same as vcxproj)
REM ----------------------------------------------------------------------
setlocal

set "VS_VER=%~1"
if "%VS_VER%"=="" set "VS_VER=vs18"

if /I "%VS_VER%"=="vs18" (
  set "CMAKE_GEN=Visual Studio 18 2026"
  set "SOLUTION_EXT=slnx"
) else if /I "%VS_VER%"=="vs17" (
  set "CMAKE_GEN=Visual Studio 17 2022"
  set "SOLUTION_EXT=sln"
) else (
  echo [error] unknown target '%VS_VER%'. use vs18 or vs17
  exit /b 1
)

set "SOURCE_DIR=%~dp0"
if "%SOURCE_DIR:~-1%"=="\" set "SOURCE_DIR=%SOURCE_DIR:~0,-1%"
set "OUTPUT_DIR=%SOURCE_DIR%\cmake_projects\%VS_VER%"

REM -- find cmake: PATH first, then the one bundled with the latest Visual Studio
set "CMAKE_EXE="
for /f "delims=" %%C in ('where cmake.exe 2^>nul') do (
  if not defined CMAKE_EXE set "CMAKE_EXE=%%C"
)
if not defined CMAKE_EXE (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "delims=" %%I in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -property installationPath') do (
      if exist "%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=%%I\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
  )
)
if not defined CMAKE_EXE (
  echo [error] cmake.exe not found. Install "C++ CMake tools for Windows" in Visual Studio or add cmake to PATH.
  exit /b 1
)

echo ----------------------------------------------------------------------
echo  CGDK10 for C++ - generate %CMAKE_GEN% projects from CMake
echo    cmake  : %CMAKE_EXE%
echo    source : %SOURCE_DIR%
echo    output : %OUTPUT_DIR%
echo ----------------------------------------------------------------------

"%CMAKE_EXE%" -S "%SOURCE_DIR%" -B "%OUTPUT_DIR%" -G "%CMAKE_GEN%" -A x64
if ERRORLEVEL 1 (
  echo [error] cmake generation failed
  exit /b 1
)
echo [ok] %OUTPUT_DIR%\CGDK10.core.%SOLUTION_EXT%

if /I "%~2"=="build" (
  "%CMAKE_EXE%" --build "%OUTPUT_DIR%" --config Debug --parallel
  if ERRORLEVEL 1 exit /b 1
  "%CMAKE_EXE%" --build "%OUTPUT_DIR%" --config Release --parallel
  if ERRORLEVEL 1 exit /b 1
)

endlocal
exit /b 0
