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
Init VMI: 116824 ms
LibVMI init succeeded, waiting for CMD!
Module List: 17780 ms
Check Keyboard Notifiers: 43601 ms
Checking SCT: 69043 ms
Walking Process List: 838061 ms
Walking Process List Open Files: 577272 ms
Walking Process List Priv Esc: 559113 ms
Virt FS Hook: 36256 ms
Netstat Ops: 29074 ms
Netdev Ops: 40134 ms
TTY Drivers: 59683 ms
Single Page: 3187 ms

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

