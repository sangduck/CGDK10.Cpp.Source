@ECHO ------------------------------------------
@ECHO               CGDK10 for C++
@ECHO             [Network Projects]
@ECHO:
@ECHO               Core Projects             
@ECHO
@ECHO        build for Visual Studio 2022       
@ECHO ------------------------------------------
@ECHO @ start Compile

@ECHO OFF
set nowpath=%cd%
call "%VS143COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86_x64
cd /D "%nowpath%"

cd src

@ECHO @ Compile Debug
devenv system\CGDK10.system.windows.vs17.sln /%3 "%1|%2"
devenv net.socket\CGDK10.net.socket.windows.vs17.sln /%3 "%1|%2"
devenv net.update\CGDK10.net.update.windows.vs17.sln /%3 "%1|%2"
devenv server.system\CGDK10.server.system.windows.vs17.sln /%3 "%1|%2"
devenv server.query\CGDK10.server.query.windows.vs17.sln /%3 "%1|%2"
devenv server.http_api\CGDK10.server.http_api.windows.vs17.sln /%3 "%1|%2"
devenv server.service\CGDK10.server.service.windows.vs17.sln /%3 "%1|%2"
devenv server.web\CGDK10.server.web.windows.vs17.sln /%3 "%1|%2"
devenv server.admin\CGDK10.server.admin.host_agent.service.windows.vs17.sln /%3 "%1|%2"

@ECHO ------------------------------------------
@ECHO @ Complete!!! (for Visual Studio 2022)
@ECHO     %1 (%2)
@ECHO ------------------------------------------

cd ..
