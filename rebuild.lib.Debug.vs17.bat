@ECHO OFF

cd build
call _build.vs17 Debug x64 rebuild
cd ..

pause