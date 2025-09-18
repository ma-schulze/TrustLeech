We expect the TrustLeech based VMI policies to be multitudes faster than the KVM based execution of the VMI policies. 

The exact duration of the execution of the policies may vary, especially for the KVM based execution, but the general results should be similar.
In our experiments, the following runtimes were observed:


## TrustLeech numbers
Init VMI: 86207 ms
LibVMI init succeeded, waiting for CMD!
Module List: 20385 ms
Check Keyboard Notifiers: 34594 ms
Checking SCT: 5736 ms
Walking Process List: 118760 ms
Walking Process List Open Files: 304460 ms
Walking Process List Priv Esc: 189055 ms
Virt FS Hook: 39675 ms
Netstat Ops: 22920 ms
Netdev Ops: 21701 ms
TTY Drivers: 313547 ms
Single Page: 201 ms


## KVM Numbers
GOT KVM!
Init VMI: 110241 ms
LibVMI init succeeded, waiting for CMD!
Module List: 26928 ms
Check Keyboard Notifiers: 20148 ms
Checking SCT: 54058 ms
Walking Process List: 712189 ms
Walking Process List Open Files: 508435 ms
Walking Process List Priv Esc: 582063 ms
Virt FS Hook: 51658 ms
Netstat Ops: 17640 ms
Netdev Ops: 50536 ms
TTY Drivers: 62710 ms
Single Page: 2552 ms

## Normalize Them
To help calculate this, you can use this python sippet:
```python3 
kvm_timings = [
    # INPUT YOUR RESULTS HERE
]

tl_timings = [
    # INPUT YOUR RESULTS HERE
]

# Normalize KVM timings to TrustLeech timings
normalized_kvm_timings = [kvm / tl for kvm, tl in zip(kvm_timings, tl_timings)]
rounded_timings = [round(x, 2) for x in normalized_kvm_timings]

print(rounded_timings)
```

This results in the following relative numbers:
```
Init VMI: 1.28
Single Page: 12.70
Module List: 1.32
Check Keyboard Notifiers: 0.58
Checking SCT: 9.42
Walking Process List: 6.00
Walking Process List Open Files: 1.67
Walking Process List Priv Esc: 3.08
Virt FS Hook: 1.30
Netstat Ops: 0.77
Netdev Ops: 2.33
TTY Drivers: 0.20
```

It must be noted, that due to different configurations of the virsh VM to make the workflow easier for the artifact evaluation, no TTY is attached to the `virsh` VM and it does not have a proper network connection. 
Therefore, other/less drivers for TTY/Keyboard, and Networking are loaded.
This results in the numbers for these policies converging from the numbers presented in the paper.
