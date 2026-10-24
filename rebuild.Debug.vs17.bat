@ECHO OFF

cd build
rd "output" /S /Q >nul
rd "lib/cgdk" /S /Q >nul
call _build.vs17 Debug x64 rebuild
cd ..

cd tutorials
rd "output" /S /Q >nul
rd "execute" /S /Q >nul
call _build.vs17 Debug x64 rebuild
cd ..

cd examples
rd "output" /S /Q >nul
rd "execute" /S /Q >nul
call _build.vs17 Debug x64 rebuild
cd ..

pause