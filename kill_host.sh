#!/bin/sh

QEMU_MONITOR_COMMAND="quit"
echo "${QEMU_MONITOR_COMMAND}" | socat - UNIX-CONNECT:outs/host_qemu_monitor