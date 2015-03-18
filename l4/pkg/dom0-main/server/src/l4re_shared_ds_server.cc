#include "l4re_shared_ds_server.h"

#include <l4/dom0-main/communication_magic_numbers.h>
#include <l4/dom0-main/ipc_protocol.h>

#include <stdio.h>

L4reSharedDsServer::L4reSharedDsServer(const char* capName,
		L4::Cap<L4Re::Dataspace>& ds) :
		ipcServer(ds)
{
	this->capName = capName;
}

bool L4reSharedDsServer::init()
{
	if (!registryServer.registry()->register_obj(&ipcServer, capName).is_valid())
	{
		printf("Could not register my service, is there a 'l4re_ipc' in the caps table?\n");
		return false;
	}
	printf("Welcome to the shared ds server for l4re!\n");
	return true;
}

void L4reSharedDsServer::loop()
{
	registryServer.loop();
}

//Tell if l4re has already finished copying the contents of the shared ds.
//If true is returned, it can be safely reused.
bool L4reSharedDsServer::dsInUse()
{
	return ipcServer.dsInUse();
}


L4reSharedDsServer::L4reIpcServer::L4reIpcServer(L4::Cap<L4Re::Dataspace>& ds) :
		dsIsInUse(false), ds(ds)
{
}

int L4reSharedDsServer::L4reIpcServer::dispatch(l4_umword_t,
		L4::Ipc::Iostream& ios)
{
	l4_msgtag_t t;
	ios >> t;
	// We're only talking the l4reIpc protocol
	if (t.label() != Protocol::l4reIpc)
		return -L4_EBADPROTO;

	L4::Opcode opcode;
	ios >> opcode;
	switch (opcode)
	{
	case Opcode::getSharedDataspace:
		//Mark the ds used until l4re has copied its content
		dsIsInUse=true;
		//Write the dataspace capability into the stream. The kernel will map it to the other task.
		ios << ds;
		return L4_EOK;
	case Opcode::dataspaceCopyFinished:
		//The l4re thread has finished copying the contents of the shared dataspaces;
		//we can now reuse it for something else.
		dsIsInUse=false;
		return L4_EOK;
	default:
		return -L4_ENOSYS;
	}
}


bool L4reSharedDsServer::L4reIpcServer::dsInUse()
{
	return dsIsInUse;
}
