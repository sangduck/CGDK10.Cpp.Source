@ECHO OFF

cd tutorials
call _build.vs18 Debug x64 build
cd ..

cd examples
call _build.vs18 Debug x64 build
cd ..

pause