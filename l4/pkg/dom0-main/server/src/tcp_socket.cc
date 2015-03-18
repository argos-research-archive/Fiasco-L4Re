#include "tcp_socket.h"


#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <lwip/ip_addr.h>



//Receive data from the socket and write it into data.
ssize_t TcpSocket::receiveData(void* data, size_t size)
{
	ssize_t result = 0;
	ssize_t position = 0;
	//Because read() might actually read less than size bytes
	//before it returns, we call it in a loop
	//until size bytes have been read.
	do
	{
		result = read(targetSocket, (char*) data + position, size - position);
		if (result < 1)
			return -errno;
		position += result;

	} while ((size_t) position < size);
	return position;
}

//convenience function
ssize_t TcpSocket::receiveInt32_t(int32_t& data)
{
	return receiveData(&data, sizeof(data));
}


//Send data from buffer data with size size to the socket.
ssize_t TcpSocket::sendData(void* data, size_t size)
{
	ssize_t result = 0;
	ssize_t position = 0;
	//Because write() might actually write less than size bytes
	//before it returns, we call it in a loop
	//until size bytes have been written.
	do
	{
		result = write(targetSocket, (char*) data + position, size - position);
		if (result < 1)
			return -errno;
		position += result;

	} while ((size_t) position < size);
	return position;
}

//convenience function
ssize_t TcpSocket::sendInt32_t(int32_t data)
{
	return sendData(&data, sizeof(data));
}
