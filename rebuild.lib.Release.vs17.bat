@ECHO OFF

cd build
call _build.vs17 Release x64 rebuild
cd ..

pause