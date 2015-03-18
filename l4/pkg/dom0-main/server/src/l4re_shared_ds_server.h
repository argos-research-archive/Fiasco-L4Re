#pragma once

#include <l4/re/util/object_registry>



//This class is used to create and run an IPC server
//which answers requests (from l4re) for a shared dataspace
class L4reSharedDsServer
{
private:
	//This is the actual IPC Server object class.
	//However, it is not needed to access this class directly (therefore private).
	class L4reIpcServer: public L4::Server_object
	{
	public:
		L4reIpcServer(L4::Cap<L4Re::Dataspace>& ds);

		int dispatch(l4_umword_t, L4::Ipc::Iostream& ios);

		bool dsInUse();

	private:
		bool dsIsInUse;
		L4::Cap<L4Re::Dataspace>& ds;
	};



public:
	L4reSharedDsServer(const char* capName, L4::Cap<L4Re::Dataspace>& ds);

	bool init();

	void loop();

	//Tell if l4re has already finished copying the contents of the shared ds.
	//If true is returned, it can be safely reused.
	bool dsInUse();

private:
	const char* capName;
	L4Re::Util::Registry_server<> registryServer;
	L4reIpcServer ipcServer;
};
