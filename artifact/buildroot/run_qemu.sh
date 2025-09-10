#!/usr/bin/env bash

set -eu

# Make sure the current working directory is the location of the script
cd "$(dirname "$0")" || exit 1

DEVICE_TREE="$(readlink -f ../device-tree/qemu_gicv4_mc.dtb)"

cd output/images || exit 1

# Remember to recompile the device tree when changing arguments here, as the
# device tree contains some of the data here, e.g. the kernel command line and
# NUMA hierarchy
../host/bin/qemu-system-aarch64 \
  -nographic \
  -machine virt,secure=on,gic-version=4,virtualization=on,iommu=smmuv3,ras=on \
  -cpu max,sve=on,sme=off,pauth-impdef=on \
  -dtb "$DEVICE_TREE" \
  -m 4G \
  -smp 4 \
  -bios qemu_fw.bios \
  -kernel Image \
  --append "kvm-arm.mode=nested kvm-arm.vgic_v4_enable=on rootwait root=/dev/vda nokaslr" \
  -semihosting-config enable=on,target=native \
  -drive file=rootfs.ext4,if=none,format=raw,id=hd0 \
  -device virtio-blk-device,drive=hd0 \
  -device qemu-xhci,id=xhci -device usb-kbd \
  -device virtio-net-pci,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::5555-:5555 \
  -plugin ../build/host-qemu-custom/build/contrib/plugins/libips.so,ips=500000000  -s
  # -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no \
  # -device virtio-net-pci,netdev=mynet0 
#   -monitor telnet::45454,server,nowait \
#  -chardev stdio,id=char0,logfile=qemu_serial.log,signal=off \
#  -serial chardev:char0 \


  # -device virtio-net-pci,netdev=net0 \
  # -netdev user,id=net0,hostfwd=tcp::5555-:5555 \
