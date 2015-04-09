PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

PC_FILENAME      = $(PKGNAME)
TARGET           = lib$(PKGNAME).a
SYSTEMS          = x86-l4f arm-l4f
SRC_CC           = dom0_server.cc l4re_shared_ds_server.cc lua_ipc_client.cc remote_dom0_instance.cc tcp_socket.cc tcp_client_socket.cc tcp_server_socket.cc 
REQUIRES_LIBS    = lwip libc_be_socket_lwip liblwip_netif_ankh libc_support_misc libc_be_file_stdin


include $(L4DIR)/mk/lib.mk

