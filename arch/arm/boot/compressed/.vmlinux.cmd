cmd_arch/arm/boot/compressed/vmlinux := arm-linux-ld -EL    --defsym zreladdr=0x50008000 --defsym params_phys=0x50000100 -p --no-undefined -X /home/xm/FriendlyARM/toolschain/4.5.1/bin/../lib/gcc/arm-none-linux-gnueabi/4.5.1/libgcc.a -T arch/arm/boot/compressed/vmlinux.lds arch/arm/boot/compressed/head.o arch/arm/boot/compressed/piggy.o arch/arm/boot/compressed/misc.o -o arch/arm/boot/compressed/vmlinux 