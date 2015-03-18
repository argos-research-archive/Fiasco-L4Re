PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

TARGET           = dom0
SRC_CC           = main.cc dom0_server.cc l4re_shared_ds_server.cc lua_ipc_client.cc remote_dom0_instance.cc tcp_socket.cc tcp_client_socket.cc tcp_server_socket.cc 
SYSTEMS          = x86-l4f arm-l4f
REQUIRES_LIBS    = lwip libc_be_socket_lwip liblwip_netif_ankh libc_support_misc libc_be_file_stdin


include $(L4DIR)/mk/prog.mk

