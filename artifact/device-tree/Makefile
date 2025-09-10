
.PHONY: all clean

all: qemu.dtb

clean:
	rm -f qemu.dtb

qemu.dtb: qemu.dts
	dtc -O dtb -o $@ $<
