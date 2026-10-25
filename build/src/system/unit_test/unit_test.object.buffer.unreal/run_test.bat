@echo off
rem ----------------------------------------------------------------------------
rem  Builds cgdk/buffers/_buffer_unreal.h with a real Unreal Engine and runs the automation test CGDK.Buffer.Unreal.
rem
rem  usage  : run_test.bat [engine version (default 5.7)] [engine root (default C:\Program Files\Epic Games\UE_[version])]
rem  result : build_[version].log, run_[version].log, exit code 0 = passed
rem ----------------------------------------------------------------------------
setlocal
set UE_VER=%~1
if "%UE_VER%"=="" set UE_VER=5.7
set UE_ROOT=%~2
if "%UE_ROOT%"=="" set UE_ROOT=C:\Program Files\Epic Games\UE_%UE_VER%
set PROJECT=%~dp0CgdkUe.uproject

call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CgdkUeEditor Win64 Development -Project="%PROJECT%" -WaitMutex -NoHotReload > "%~dp0build_%UE_VER%.log" 2>&1
if errorlevel 1 (
	echo [FAIL] build UE %UE_VER% : see build_%UE_VER%.log
	exit /b 1
)

"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT%" -ExecCmds="Automation RunTests CGDK.Buffer.Unreal" -TestExit="Automation Test Queue Empty" -nullrhi -unattended -nosplash -nop4 -nosound -stdout -FullStdOutLogOutput > "%~dp0run_%UE_VER%.log" 2>&1

findstr /C:"Result={Success} Name={Unreal} Path={CGDK.Buffer.Unreal}" "%~dp0run_%UE_VER%.log" > nul
if errorlevel 1 (
	echo [FAIL] test UE %UE_VER% : see run_%UE_VER%.log
	exit /b 1
)
echo [OK] UE %UE_VER% : CGDK.Buffer.Unreal passed
exit /b 0
