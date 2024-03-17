#ifndef PCH_H
#define PCH_H

#include "framework.h"

const uint32_t	CGNOTIFY_SOCKET_UDP_ADD		 = (eMESSAGE::USER | 8);
const uint32_t	CGNOTIFY_SOCKET_UDP_REMOVE	 = (eMESSAGE::USER | 9);
const uint32_t	CGNOTIFY_SOCKET_UDP_UPDATE	 = (eMESSAGE::USER | 10);

extern own_ptr<test_udp_echo_client> g_ptest_udp_echo_client;

#endif //PCH_H
