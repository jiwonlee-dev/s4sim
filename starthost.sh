#!/bin/sh

mkdir -p outs

Host_hw_platforms/qemu/qemu-2.8.1.1/x86_64-softmmu/qemu-system-x86_64 -m 1G -smp 4 \
-net nic,model=e1000,netdev=n0 -netdev user,hostfwd=tcp::7522-:22,id=n0  \
-hda disk_images/host.img \
-monitor unix:outs/host_qemu_monitor,server,nowait \
-vga virtio -nographic -vnc :2 -pidfile outs/host_qemu_pid &
