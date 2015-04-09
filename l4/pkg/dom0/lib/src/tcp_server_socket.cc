#include <l4/dom0/tcp_server_socket.h>

#include <unistd.h>
#include <lwip-util.h>


TcpServerSocket::TcpServerSocket(ip_addr_t address, int port)
{
	ownSocket = 0;
	targetSocket = 0;
	connected = false;
	targetSockaddr_inSize = sizeof(targetSockaddr_in);
	ownSockaddr_in.sin_family = AF_INET;
	ownSockaddr_in.sin_port = htons(port);
	ownSockaddr_in.sin_addr.s_addr = address.addr;
	int error;
	ownSocket = socket(PF_INET, SOCK_STREAM, 0);
	error = bind(ownSocket, (struct sockaddr*) (&ownSockaddr_in),
			sizeof(ownSockaddr_in));
	printf("Server: bound to addr: %d\n", error);
	error = listen(ownSocket, 10);
	printf("Server: listen(): %d\n", error);
}

TcpServerSocket::~TcpServerSocket()
{
	disconnect();
}

int TcpServerSocket::connect()
{
	targetSocket = ::accept(ownSocket, (struct sockaddr*) (&targetSockaddr_in),
			&targetSockaddr_inSize);
	printf("Got connection from ");
	lwip_util_print_ip((ip_addr_t*) (&targetSockaddr_in.sin_addr));
	printf("\n");
	connected = true;
	return targetSocket == -1 ? -errno : targetSocket;
}

int TcpServerSocket::disconnect()
{
	close(targetSocket);
	perror("a");
	close(ownSocket);
	perror("a");
	connected = false;
	return 1;
}
