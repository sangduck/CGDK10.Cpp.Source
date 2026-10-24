@ECHO ----------------------------------------------------------------------
@ECHO                            CGDK10 for C++
@ECHO                           [Network Projects]
@ECHO:
@ECHO                            Example Projects
@ECHO:
@ECHO                    build for Visual Studio 2026       
@ECHO ----------------------------------------------------------------------
@ECHO @ start Compile

@ECHO OFF
set nowpath=%cd%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /D "%nowpath%"

@ECHO @ Compile Debug
devenv 1.1.tcp_echo\CGDK10.tcp_echo_server.vs18.slnx /%3 "%1|%2"
devenv 1.1.tcp_echo\CGDK10.tcp_echo_client.vs18.slnx /%3 "%1|%2"
devenv 1.2.tcp_relay_echo\CGDK10.tcp_relay_echo.server.vs18.slnx /%3 "%1|%2"
devenv 1.2.tcp_relay_echo\CGDK10.tcp_relay_echo.client.vs18.slnx /%3 "%1|%2"
devenv 1.3.tcp_multicast_echo\CGDK10.tcp_multicast_echo.server.vs18.slnx /%3 "%1|%2"
devenv 1.3.tcp_multicast_echo\CGDK10.tcp_multicast_echo.client.vs18.slnx /%3 "%1|%2"
devenv 2.1.chatting_simple\CGDK10.chatting.server.vs18.slnx /%3 "%1|%2"
devenv 2.1.chatting_simple\CGDK10.chatting.client.vs18.slnx /%3 "%1|%2"
devenv 2.2.chatting_scaleout\CGDK10.chatting.server.channel_manager.vs18.slnx /%3 "%1|%2"
devenv 2.2.chatting_scaleout\CGDK10.chatting.server.channel.vs18.slnx /%3 "%1|%2"
devenv 2.2.chatting_scaleout\CGDK10.chatting.client.vs18.slnx /%3 "%1|%2"
devenv 6.1.game_mmo_field_1\CGDK10.game_mmo_field.server.vs18.slnx /%3 "%1|%2"
devenv 6.1.game_mmo_field_1\CGDK10.game_mmo_field.client.vs18.slnx /%3 "%1|%2"
devenv 7.1.update_service\CGDK10.update_service.server.vs18.slnx /%3 "%1|%2"
devenv 7.1.update_service\CGDK10.update_service.client.vs18.slnx /%3 "%1|%2"

@ECHO ------------------------------------------
@ECHO              Example Projects             
@ECHO @ Complete!!! (for Visual Studio 2026)
@ECHO     %1 (%2)
@ECHO ------------------------------------------
