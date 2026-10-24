@ECHO OFF

cd tutorials
call _build.vs17 Debug x64 rebuild
cd ..

cd examples
call _build.vs17 Debug x64 rebuild
cd ..

pause