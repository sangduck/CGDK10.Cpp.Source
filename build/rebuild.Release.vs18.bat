@ECHO OFF
rd "output" /S /Q >nul
rd "lib/cgdk" /S /Q >nul
call _build.vs18 Release x64 rebuild
pause