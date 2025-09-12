Since the memory range we dump is outside of the TrustLeech-hypervisor, we expect the dumps to be equal:
```
❯ ./eval.sh
Requirement already satisfied: numpy in ./venv/lib/python3.13/site-packages (2.3.3)
Starting
Processing QEMU dump
len pq: 8192, len tl: 8192
Diffs: 0
Diffs from TL-Hypervisor: 0
```
