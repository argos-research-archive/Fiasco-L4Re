#!/bin/sh
#cp l4re-snapshot/obj/l4/x86/conf/Makeconf.boot.default l4re-snapshot/obj/l4/x86/conf/Makeconf.boot
#cat Makeconf.boot.add >> l4re-snapshot/obj/l4/x86/conf/Makeconf.boot

QEMU_OPTIONS="-net socket,listen=:8010" \
MAC="52:54:00:00:00:01" \
make -C ../obj/l4 qemu E=dom0_server_only &

sleep 1

QEMU_OPTIONS="-net socket,connect=127.0.0.1:8010" \
MAC="52:54:00:00:00:02" \
make -C ../obj/l4 qemu E=dom0_server_and_client &
