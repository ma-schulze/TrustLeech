import numpy as np


pages_qemu = []
pages_tl = []

print("Starting")

with open("./data/qemu_memdump_good.txt", "rb") as f_qemu:
    while page_qemu := f_qemu.read(4096):
        pages_qemu.append(page_qemu)

print("Processing QEMU dump")

with open("./data/output2_good_sorted.txt", "rb") as f_tl:
    while page_tl := f_tl.read(4096):
        pages_tl.append(page_tl)
    
print(f"len pq: {pages_qemu.__len__()}, len tl: {pages_tl.__len__()}")

def compare_memory_dumps(dump1, dump2):
    differences = []
    max_length = max(len(dump1), len(dump2))

    for i in range(max_length):
        if i < len(dump1) and i < len(dump2):
            differences.append(dump1[i] != dump2[i])
        else:
            differences.append(True)  # Difference if one of the dumps doesn't have this block

    return differences

def analyze_differences(differences):
    d = 0
    d2 = 0

    for index, difference in enumerate(differences):
        if difference:
            d += 1
            print(index)
            if 0xC0000000 < (index * 4096) < (0xC0000000 + 0x20000000):
                d2 += 1
    print(f"Diffs: {d}")
    print(f"Diffs from TL-Hypervisor: {d2}")


differences = compare_memory_dumps(pages_qemu, pages_tl)
analyze_differences(differences)

