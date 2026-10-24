@ECHO OFF
call _build.vs17 Debug x64 build
call _build.vs17 Release x64 build
pause