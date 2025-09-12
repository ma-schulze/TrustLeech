We expect the TrustLeech based VMI policies to be multitudes faster than the KVM based execution of the VMI policies. 

The exact duration of the execution of the policies may vary, especially for the KVM based execution, but the general results should be similar.
In our experiments, the following runtimes of the KVM backend, normalzed to the TrustLeech backend, were observed:

Init: 4.77
P1: 3.28
P2: 1.98
P3: 4.28
P4: 9.76
P5: 10.24
P6: 9.83
P7: 8.49
P8: 5.63 
P9: 6.17
P10: 7.13
P11: 1.79

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

