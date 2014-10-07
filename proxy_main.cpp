// proxy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include <WinSock2.h>

#include "proxy.h"
int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA	ws;
	WSAStartup(MAKEWORD(2, 2), &ws);;

	struct proxy *prox;

	prox = proxy_create();
	assert(prox);


	proxy_set(prox, "192.168.79.1", 1080, "test", "0");
	proxy_connect(prox, "113.108.16.44", 25);
	proxy_recv_line(prox, buff, 512);

	return 0;
}

