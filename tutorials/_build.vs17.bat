@ECHO ----------------------------------------------------------------------
@ECHO                            CGDK10 for C++
@ECHO                           [Network Projects]
@ECHO:
@ECHO                            Tutorial Projects
@ECHO:
@ECHO                    build for Visual Studio 2022       
@ECHO ----------------------------------------------------------------------
@ECHO @ start Compile

@ECHO OFF
set nowpath=%cd%
call "%VS143COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat"
cd /D "%nowpath%"

@ECHO @ Compile Debug
devenv common.1.object\CGDK10.tutorial.object.vs17.sln /%3 "%1|%2"
devenv common.2.buffer\CGDK10.tutorial.buffer.vs17.sln /%3 "%1|%2"
devenv common.3.execute\CGDK10.tutorial.execute.vs17.sln /%3 "%1|%2"
devenv network.1.socket\CGDK10.tutorial.socket.server.vs17.sln /%3 "%1|%2"
devenv network.1.socket\CGDK10.tutorial.socket.client.vs17.sln /%3 "%1|%2"
devenv server.1.log\CGDK10.tutorial.server.log.vs17.sln /%3 "%1|%2"
devenv server.2.group\CGDK10.tutorial.server.group.server.vs17.sln /%3 "%1|%2"
devenv server.2.group\CGDK10.tutorial.server.group.client.vs17.sln /%3 "%1|%2"
devenv server.3.service\CGDK10.tutorial.server.service.vs17.sln /%3 "%1|%2"
devenv server.4.time_event\CGDK10.tutorial.server.time_event.vs17.sln /%3 "%1|%2"
devenv server.5.query\CGDK10.tutorial.query.vs17.sln /%3 "%1|%2"
REM devenv server.6.oauth\CGDK10.tutorial.server.oauth.vs17.sln /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.server.storage.vs17.sln /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.server.session.vs17.sln /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.client.vs17.sln /%3 "%1|%2"
devenv server.8.web_server\CGDK10.tutorial.server.web_server.vs17.sln /%3 "%1|%2"

@ECHO ------------------------------------------
@ECHO              Tutorial Projects             
@ECHO @ Complete!!! (for Visual Studio 2022)
@ECHO     %1 (%2)
@ECHO ------------------------------------------
