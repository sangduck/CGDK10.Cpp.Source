@ECHO OFF

cd build
call _build.vs18 Debug x64 build
call _build.vs18 Release x64 build
cd ..

cd tutorials
call _build.vs18 Debug x64 build
call _build.vs18 Release x64 build
cd ..

cd examples
call _build.vs18 Debug x64 build
call _build.vs18 Release x64 build
cd ..

pause