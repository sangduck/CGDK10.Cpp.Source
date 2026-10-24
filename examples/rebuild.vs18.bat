@ECHO OFF
call _build.vs18 Debug x64 rebuild
call _build.vs18 Release x64 rebuild
pause