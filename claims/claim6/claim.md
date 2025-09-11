# Claim 6 -TCB

These testcases evaluate the TCB of TrustLeech.
To evaluate the additions, we rely on the tool `cloc`. 

## TCB Additions to TF-A

The additions to TF-A are contained in `./artifact/arm-trusted-firmware/services/trustleech_svc/ `
We evaluate the LoC using:
```
cloc ./artifact/arm-trusted-firmware/services/trustleech_svc/ 
```

## TCB of TrustLeech Hypervisor
For the rust virtio-driver:
`cloc --include-ext=rs ./src/drivers/virtio-drivers/src/ ./src/drivers/virtio-drivers/examples/aarch64/src/main.rs`

For the TrustLeech Hypervisor:
```
cd artifact/trustleech/src/drivers 
cloc --exclude-dir=virtio-drivers --include-ext=S ../ 
cloc --exclude-dir=virtio-drivers --include-ext=c,cpp,h,hpp ../ ../../arm-trusted-firmware/plat/qemu/trustleech_exp.h
```
