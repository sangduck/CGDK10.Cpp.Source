@ECHO OFF

cd tutorials
call _build.vs18 Debug x64 rebuild
cd ..

cd examples
call _build.vs18 Debug x64 rebuild
cd ..

pause