#pragma once

#include <l4/sys/capability>
#include <l4/re/dataspace>
#include <lwip/ip_addr.h>
#include "lua_ipc_client.h"
#include "l4re_shared_ds_server.h"

//Since pthread_create only accepts functions with a single void* argument,
//all arguments need to be packed into a structure.

struct clientThreadArgs
{
	ip_addr_t address;
	int port;
	LuaIpcClient* luaIpcClient;
};

struct serverThreadArgs
{
	ip_addr_t address;
	int port;
	L4::Cap<L4Re::Dataspace>* dataspace;
	LuaIpcClient* luaIpcClient;
	L4reSharedDsServer* dsServer;
};
