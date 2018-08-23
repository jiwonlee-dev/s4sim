#!/bin/sh

QEMU_MONITOR_COMMAND="device_add nvme,serial=foo,s4sim_shm=s4sim_id,id=s4sim_id"
echo "${QEMU_MONITOR_COMMAND}" | socat - UNIX-CONNECT:outs/host_qemu_monitor