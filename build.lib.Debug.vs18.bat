@ECHO OFF

cd build
call _build.vs18 Debug x64 build
cd ..

pause