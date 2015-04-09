//TCP tuning
#define TCP_MSS 1460
#define TCP_WND TCP_MSS*16
#define TCP_SND_BUF TCP_WND

/*
 * Need to include this file before others.
 * Sets our byteorder.
 */
extern "C"
{
#include "arch/cc.h"
}

#include <l4/re/dataspace>
#include <l4/util/util.h>
#include <l4/ankh/client-c.h>
#include <l4/ankh/session.h>
#include <l4/ankh/lwip-ankh.h>
#include <lwip/ip4_addr.h>
#include <arpa/inet.h>
#include <netif/etharp.h>
#include <lwip-util.h>

#include <getopt.h>
#include <stdio.h>
#include <pthread-l4.h>


#include <l4/dom0/remote_dom0_instance.h>
#include <l4/dom0/dom0_server.h>
#include <l4/dom0/thread_args.h>
#include <l4/dom0/l4re_shared_ds_server.h>
#include <l4/dom0/lua_ipc_client.h>
#include <l4/dom0/tcp_server_socket.h>
#include <l4/dom0/tcp_client_socket.h>
#include <l4/dom0/communication_magic_numbers.h>
#include <l4/dom0/ipc_protocol.h>

#include <stdio.h>
#include <iostream>


#define GETOPT_LIST_END { 0, 0, 0, 0 }

enum options
{
	BUF_SIZE,
	SHM_NAME,
	ENABLE_CLIENT,
	LISTEN_ADDRESS,
	LISTEN_PORT,
	GATEWAY,
	NETMASK,
	TARGET_ADDRESS,
	TARGET_PORT
};

extern "C"
{
extern err_t
ankhif_init(struct netif*);
}

static ankh_config_info cfg =
{ 1024, L4_INVALID_CAP, L4_INVALID_CAP, "" };




static void* clientDemo(void* args)
{
	printf("Client mode\n");

	//luaIpc processes LUA commands for the local machine
	LuaIpcClient& luaIpc = *((clientThreadArgs*) args)->luaIpcClient;

	//remoteDom0 processes LUA commands for the remote machine
	//and receives binaries
	RemoteDom0Instance remoteDom0(((clientThreadArgs*) args)->address,
			((clientThreadArgs*) args)->port);
	remoteDom0.connect();



	//DEMO CODE STARTS HERE
	printf("********DEMO*******\n");
	l4_sleep(3000);
	printf("Sending a simple calculation to remote server in 10 seconds\n");
	l4_sleep(10000);
	printf("send...\n");
	remoteDom0.executeLuaString("print(3+5)");
	printf("sent.\n");
	l4_sleep(5000);
	printf(	"In 10 seconds: start hello on this local server. After 10 seconds, "
					"send remote server a request to also start hello and "
					"immediately kill hello on this local server. "
					"(migration)\n"
					"kill it after another 10 seconds.\n");
	l4_sleep(10000);
	printf("start.\n");
	luaIpc.executeString("hello = L4.default_loader:start({},\"rom/hello\");");
	l4_sleep(10000);
	printf("migrating.\n");
	remoteDom0.executeLuaString("a = L4.default_loader:start({},\"rom/hello\");");
	printf("kill...\n");
	luaIpc.executeString("hello:kill()");
	l4_sleep(10000);
	printf("kill.\n");
	remoteDom0.executeLuaString("a:kill()");

	l4_sleep(10000);

	printf("Now, the same with a binary that is NOT yet present on the remote host:\n"
								"In 10 seconds: start test on this local server. "
								"After 10 seconds, send remote server the \"test\" binary. "
								"After completing, wait 10 seconds, then "
								"send a request to start the received binary and "
								"immediately kill test on this local server."
								"(migration).\n"
								"kill it after another 10 seconds.\n");
	l4_sleep(10000);
	printf("start.\n");
	luaIpc.executeString("test = L4.default_loader:start({},\"rom/test\");");
	l4_sleep(10000);
	printf("sending binary...\n");
	remoteDom0.sendElfBinaryFromRom("rom/test");
	printf("sent binary.\n");
	printf("In 10 seconds, start this binary remotely and kill it locally.\n");
	l4_sleep(10000);
	printf("start.\n");
	remoteDom0.executeLuaString("test = L4.default_loader:start({caps={l4re_ipc = L4.Env.l4re_ipc}},\"network\");");
	luaIpc.executeString("test:kill();");
	l4_sleep(10000);
	remoteDom0.executeLuaString("test:kill()");
	l4_sleep(3000);
	printf("********DEMO FINISHED*******\n");


	l4_sleep_forever();

	return NULL;
}

int main(int argc, char **argv)
{
	//Initialize stuff
	L4::Cap<L4Re::Dataspace> ds;
	LuaIpcClient luaIpc("lua_ipc");
	struct netif my_netif;
	int listenPort = 0;
	int targetPort = 0;
	ip_addr_t localAddress;
	ip_addr_t targetAddress;
	ip_addr_t gateway;
	ip_addr_t netmask;
	localAddress.addr = 0;
	targetAddress.addr = 0;
	gateway.addr = 0;
	netmask.addr = 0;
	pthread_t serverLuaThread = NULL;
	pthread_t clientThread = NULL;
	struct clientThreadArgs clientArgs;
	struct serverThreadArgs serverArgs;
	bool enableClient = false;

	luaIpc.init();
	if (l4ankh_init())
		return 1;
	l4_cap_idx_t c = pthread_getl4cap(pthread_self());
	cfg.send_thread = c;
	cfg.recv_thread = c;


	//Process arguments, check for invalid values
	static struct option long_opts[] =
	{
	{ "bufsize", required_argument, 0, BUF_SIZE },
	{ "shm", required_argument, 0, SHM_NAME },
	{ "enable-client", no_argument, 0, ENABLE_CLIENT },
	{ "listen-address", required_argument, 0, LISTEN_ADDRESS },
	{ "listen-port", required_argument, 0, LISTEN_PORT },
	{ "gateway", required_argument, 0, GATEWAY },
	{ "netmask", required_argument, 0, NETMASK },
	{ "target-address", required_argument, 0, TARGET_ADDRESS },
	{ "target-port", required_argument, 0, TARGET_PORT },
	GETOPT_LIST_END };

	while (1)
	{
		int optind = 0;
		int opt = getopt_long(argc, argv, ""/*"b:s:ci:l:g:n:t:p:"*/, long_opts,
				&optind);
		printf("getopt: %d\n", opt);

		if (opt == -1)
			break;

		switch (opt)
		{
		case BUF_SIZE:
			printf("buf size: %d\n", atoi(optarg));
			cfg.bufsize = atoi(optarg);
			break;
		case SHM_NAME:
			printf("shm name: %s\n", optarg);
			snprintf(cfg.shm_name, CFG_SHM_NAME_SIZE, "%s", optarg);
			break;
		case ENABLE_CLIENT:
			printf("Client enabled\n");
			enableClient = true;
			break;
		case LISTEN_ADDRESS:
			printf("Interface and listen address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &localAddress))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;
		case TARGET_ADDRESS:
			printf("Target address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &targetAddress))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;
		case GATEWAY:
			printf("Gateway address: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &gateway))
			{
				printf("Not a valid IPv4 address. Exiting.\n");
				return -1;
			}
			break;
		case NETMASK:
			printf("Netmask: %s\n", optarg);
			if (!inet_aton(optarg, (in_addr*) &netmask))
			{
				printf("Not a valid netmask. Exiting.\n");
				return -1;
			}
			break;
		case LISTEN_PORT:
			printf("Listen port: %d\n", atoi(optarg));
			listenPort = atoi(optarg);
			break;
		case TARGET_PORT:
			printf("Target port: %d\n", atoi(optarg));
			targetPort = atoi(optarg);
			break;
		default:
			break;
		}
	}
	if (cfg.bufsize == 0)
	{
		printf("No valid buffer size specified. Exiting.\n");
		return -1;
	}
	if (cfg.shm_name[0] == '\0')
	{
		printf("Empty shm name. Exiting.\n");
		return -1;
	}
	if (localAddress.addr == 0)
	{
		printf("No valid local address specified. Exiting.\n");
		return -1;
	}
	if (netmask.addr == 0)
	{
		printf("No valid netmask specified. Exiting.\n");
		return -1;
	}
	if (gateway.addr == 0)
	{
		printf("No valid gateway specified. Exiting.\n");
		return -1;
	}
	if (listenPort == 0)
	{
		printf("No valid LUA bind port specified. Exiting.\n");
		return -1;
	}
	if (enableClient)
	{
		if (targetAddress.addr == 0)
		{
			printf("No valid target address specified. Exiting.\n");
			return -1;
		}
		if (targetPort == 0)
		{
			printf("No valid target LUA port specified. Exiting.\n");
			return -1;
		}
	}

	// Start the TCP/IP thread & init remaining networking stuff
	tcpip_init(NULL, NULL);
	struct netif *n = netif_add(&my_netif, &localAddress, &netmask, &gateway,
			&cfg, // configuration state
			ankhif_init, ethernet_input);
	printf("netif_add: %p (%p)\n", n, &my_netif);
	assert(n == &my_netif);
	netif_set_default(&my_netif);
	netif_set_up(&my_netif);
	while (!netif_is_up(&my_netif))
		l4_sleep(1000);
	printf("Network interface is up.\n");
	printf("IP: ");
	lwip_util_print_ip(&my_netif.ip_addr);
	printf("\n");
	printf("GW: ");
	lwip_util_print_ip(&my_netif.gw);
	printf("\n");


	ds = L4Re::Util::cap_alloc.alloc<L4Re::Dataspace>();
	if (!ds.is_valid())
	{
		printf("Dataspace allocation failed.\n");
		return -1;
	}

	if (enableClient)
	{
		//start the client demo in a new thread
		clientArgs.address = targetAddress;
		clientArgs.port = targetPort;
		clientArgs.luaIpcClient = &luaIpc;
		pthread_create(&clientThread, NULL, clientDemo, &clientArgs);
	}

	{
		L4reSharedDsServer l4reSharedDsServer("l4re_ipc", ds);

		//start the dom0 server thread
		serverArgs.address = localAddress;
		serverArgs.port = listenPort;
		serverArgs.dataspace = &ds;
		serverArgs.luaIpcClient = &luaIpc;
		serverArgs.dsServer = &l4reSharedDsServer;
		pthread_create(&serverLuaThread, NULL, dom0Server, &serverArgs);

		//start the l4re ipc server for answering shared dataspace requests
		l4reSharedDsServer.init();
		l4reSharedDsServer.loop();
	}


	l4_sleep_forever();

	return 0;
}
