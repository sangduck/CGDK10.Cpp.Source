@ECHO OFF
rd "output" /S /Q >nul
rd "lib/cgdk" /S /Q >nul

call _build.vs18 Debug x64 rebuild
pause