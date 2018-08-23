#!/bin/sh

QEMU_MONITOR_COMMAND="device_del s4sim_id"
echo "${QEMU_MONITOR_COMMAND}" | socat - UNIX-CONNECT:outs/host_qemu_monitor