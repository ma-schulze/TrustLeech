#!/bin/bash 

cd ../../artifact/buildroot/ 
if [ ! -f ../overlay/rootfs.ext4 ]; then
    cp output/images/rootfs.ext4 ../overlay/
fi
./run_qemu_2_1.sh
