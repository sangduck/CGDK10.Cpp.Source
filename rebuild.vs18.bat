@ECHO OFF

cd build
call _build.vs18 Debug x64 rebuild
call _build.vs18 Release x64 rebuild
cd ..

cd tutorials
call _build.vs18 Debug x64 rebuild
call _build.vs18 Release x64 rebuild
cd ..

cd examples
call _build.vs18 Debug x64 rebuild
call _build.vs18 Release x64 rebuild
cd ..

pause