When sending the signal, the VMI application in the second QEMU Environment should detect this. 
On detection, it prints the name of the according userspace app sending the singal (`sh` in our case, since its a build-in for busybox).
