This folder contains necessary files to boot rpi-os up. They are downloaded from <https://github.com/raspberrypi/firmware/tree/8c7c52466505df5d420a5cb9131ec29205bcecf8/boot>.

`armstub8-rpi4.bin` is compiled from `armstub8.S` by following make rules:

```makefile
%8-rpi4.o: %8.S
	$(CC) -DBCM2711=1 -c $< -o $@

%8-rpi4.elf: %8-rpi4.o
	$(LD) --section-start=.text=0 $< -o $@

%8-rpi4.tmp: %8-rpi4.elf
	$(OBJCOPY) $< -O binary $@

%8-rpi4.bin: %8-rpi4.tmp
	dd if=$< ibs=256 of=$@ conv=sync
```
