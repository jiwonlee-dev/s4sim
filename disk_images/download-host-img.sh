#!/bin/sh

wget https://github.com/snp-lab/s4sim/raw/images/debian-host.qcow2.xz.aa
wget https://github.com/snp-lab/s4sim/raw/images/debian-host.qcow2.xz.ab
wget https://github.com/snp-lab/s4sim/raw/images/debian-host.qcow2.xz.ac
wget https://github.com/snp-lab/s4sim/raw/images/debian-host.qcow2.xz.ad
cat debian-host.qcow2.xz.* > debian-host.qcow2.xz
unxz debian-host.qcow2.xz
ln -s debian-host.qcow2 host.img
rm debian-host.qcow2.xz.a*

