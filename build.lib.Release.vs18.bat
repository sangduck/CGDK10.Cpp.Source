@ECHO OFF

cd build
call _build.vs18 Release x64 build
cd ..

pause
