# The Makefile components that are part of most every project.

PREFIX  = riscv64-unknown-elf-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc -c
LD      = $(PREFIX)gcc
AR      = $(PREFIX)ar
RANLIB  = $(PREFIX)ranlib
OBJDUMP = $(PREFIX)objdump
OBJCOPY = $(PREFIX)objcopy
NM      = $(PREFIX)nm

QEMU    = qemu-system-riscv${BITS}

# -O just balances the compiler getting too crazy or being too silly.
# -f and -g options improve debuggability.
CFLAGS += \
   -Wno-implicit-int \
   -Wno-implicit-function-declaration \
   -Wno-return-type \
   -MD \
   -O \
   -fno-omit-frame-pointer \
   -ggdb 

LDFLAGS += \
   -Wl,--gc-sections \
   -nostartfiles \
   -nodefaultlibs 

ifeq ($(BITS),32)
  CFLAGS+=-march=rv32i -mabi=ilp32 -mcmodel=medany
  ASFLAGS+=-march=rv32i -mabi=ilp32 -mcmodel=medany
  LDFLAGS+=-march=rv32i -mabi=ilp32 -mcmodel=medany
endif