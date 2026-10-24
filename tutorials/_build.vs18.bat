@ECHO ----------------------------------------------------------------------
@ECHO                            CGDK10 for C++
@ECHO                           [Network Projects]
@ECHO:
@ECHO                            Tutorial Projects
@ECHO:
@ECHO                    build for Visual Studio 2026       
@ECHO ----------------------------------------------------------------------
@ECHO @ start Compile

@ECHO OFF
set nowpath=%cd%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /D "%nowpath%"

@ECHO @ Compile Debug
devenv common.1.object\CGDK10.tutorial.object.vs18.slnx /%3 "%1|%2"
devenv common.2.buffer\CGDK10.tutorial.buffer.vs18.slnx /%3 "%1|%2"
devenv common.3.execute\CGDK10.tutorial.execute.vs18.slnx /%3 "%1|%2"
devenv network.1.socket\CGDK10.tutorial.socket.server.vs18.slnx /%3 "%1|%2"
devenv network.1.socket\CGDK10.tutorial.socket.client.vs18.slnx /%3 "%1|%2"
devenv server.1.log\CGDK10.tutorial.server.log.vs18.slnx /%3 "%1|%2"
devenv server.2.group\CGDK10.tutorial.server.group.server.vs18.slnx /%3 "%1|%2"
devenv server.2.group\CGDK10.tutorial.server.group.client.vs18.slnx /%3 "%1|%2"
devenv server.3.service\CGDK10.tutorial.server.service.vs18.slnx /%3 "%1|%2"
devenv server.4.time_event\CGDK10.tutorial.server.time_event.vs18.slnx /%3 "%1|%2"
devenv server.5.query\CGDK10.tutorial.query.vs18.slnx /%3 "%1|%2"
REM devenv server.6.oauth\CGDK10.tutorial.server.oauth.vs18.slnx /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.server.storage.vs18.slnx /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.server.session.vs18.slnx /%3 "%1|%2"
devenv server.7.doss\CGDK10.tutorial.doss.client.vs18.slnx /%3 "%1|%2"
devenv server.8.web_server\CGDK10.tutorial.server.web_server.vs18.slnx /%3 "%1|%2"

@ECHO ------------------------------------------
@ECHO              Tutorial Projects             
@ECHO @ Complete!!! (for Visual Studio 2026)
@ECHO     %1 (%2)
@ECHO ------------------------------------------
