@ECHO OFF

cd build
call _build.vs18 Debug x64 rebuild
cd ..

pause