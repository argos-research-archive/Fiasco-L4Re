#include <l4/dom0/remote_dom0_instance.h>

#include <l4/dom0/communication_magic_numbers.h>

int32_t RemoteDom0Instance::loadFile(const char* filename, char*& buffer)
{
	//Copy binary into memory
	FILE *fp;
	size_t size, read;
	fp = fopen(filename, "r");
	if (!fp)
	{
		perror("Fehler");
		return -1;
	}

	//Get filesize
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	printf("Reading file\nsize:\t%u Bytes\n", size);

	//Allocate memory and copy file content into buffer
	buffer = (char*) malloc(size);
	if (!buffer)
	{
		perror("Fehler");
		return -1;
	}
	read = fread(buffer, 1, size, fp);
	printf("read:\t%u Bytes\n", read);
	fclose(fp);
	if (read == size)
		printf("File loaded into memory.\n");
	return size;
}


RemoteDom0Instance::RemoteDom0Instance(ip_addr_t address, int port) :
		remoteServerSocket(address, port)
{
}

int RemoteDom0Instance::connect()
{
	return remoteServerSocket.connect();
}

int RemoteDom0Instance::disconnect()
{
	return remoteServerSocket.disconnect();
}

//Send a LUA command to the remote machine.
int32_t RemoteDom0Instance::executeLuaString(const char* string)
{
	return executeLuaString(string, strlen(string) + 1);
}

int32_t RemoteDom0Instance::executeLuaString(const char* string,
		int32_t size)
{
	int32_t result;
	//First, identify the message as LUA command
	if (remoteServerSocket.sendInt32_t(LUA) < 1)
		return -errno;

	//Send the size
	if (remoteServerSocket.sendInt32_t(size) < 1)
		return -errno;

	//Send the command.
	if (remoteServerSocket.sendData((void*) string, size) < 1)
		return -errno;

	//The server responds if its Ned got the whole message.
	//This tells nothing about the success of the
	//execution of the command.
	if (remoteServerSocket.receiveInt32_t(result) < 1)
		return -errno;

	return result;
}

//Sends a buffer containing an ELF binary
int32_t RemoteDom0Instance::sendElfBinaryFromMemory(char* data,
		size_t size)
{
	int32_t message;
	//Identify the type of our message
	if (remoteServerSocket.sendInt32_t(CONTROL) < 1)
		return -errno;

	if (remoteServerSocket.sendInt32_t(SEND_BINARY) < 1)
		return -errno;

	//Send the binary size
	if (remoteServerSocket.sendInt32_t((int32_t) size) < 1)
		return -errno;

	//Wait for the "GO" from the server
	if (remoteServerSocket.receiveInt32_t(message) < 1)
		return -errno;

	//Send it
	if (remoteServerSocket.sendData(data, (int32_t) size) < 1)
		return -errno;
	else
		return size;
}

//Sends a file containing an ELF binary
int32_t RemoteDom0Instance::sendElfBinaryFromRom(
		const char* localFilename)
{
	char* data = NULL;
	int32_t message = loadFile(localFilename, data);
	message = sendElfBinaryFromMemory(data, message);
	printf("Dataspace beginning and end content: %.4s %x %x %x\n",data,data[message-3],data[message-2],data[message-1]);
	free(data);
	return message;
}
