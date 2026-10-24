@ECHO OFF

call _build.vs18 Debug.NDI x64 build
call _build.vs18 Release.NDI x64 build

pause