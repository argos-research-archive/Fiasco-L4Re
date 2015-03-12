#pragma once

#include "tcp_socket.h"

#include <lwip/ip4_addr.h>


class TcpServerSocket: public TcpSocket
{
public:
	TcpServerSocket(ip_addr_t address, int port);

	~TcpServerSocket();

	int connect();

	int disconnect();

private:
	int ownSocket;
	struct sockaddr_in ownSockaddr_in;
	socklen_t targetSockaddr_inSize;

};
