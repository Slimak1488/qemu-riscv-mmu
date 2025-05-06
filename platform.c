// Platform-specific code. Could be used to shim printing to a UART or
// semihosting or something.
// This is for QEMU -machine virt -bios none -nographic

#include "riscv_encoding.h"
#include "platform.h"

#define VIRT_UART 0x10000000

#define MSTATUS_MPP_S (1 << 11)  // MPP=1 (Supervisor)
#define MSTATUS_SUM   (1 << 18)  // SUM=1

#define SET_MEPC_SAFE(addr)                             \
    do {                                                \
        uint64_t __mepc_val = ((uint64_t)(addr)) & ~0x3UL; \
        write_csr(mepc, __mepc_val);                    \
        uint64_t __check = read_csr(mepc);              \
        if (__check != __mepc_val) {                    \
            printf("Warning: mepc instaled incorrectly! 0x%lx != 0x%lx\n", __check, __mepc_val); \
        } else {                                        \
            printf("mepc: 0x%lx\n", __mepc_val); \
        }                                               \
    } while (0)

void putc(char c) {
    *(volatile char*) VIRT_UART = c;
}

void puts(char* message) {
  while (*message) {
    putc(*message++);
  }
}

extern uint64_t smode_main();

void enter_smode() {
  extern void trap(void);

  write_csr(mtvec, trap);
  printf("mtvec: 0x%lx \n", read_csr(mtvec));

  write_csr(mie, (1 << 13) | (1 << 12) | (1 << 9) | (1 << 5) | (1 << 3));
  printf("mie: 0x%lx \n", read_csr(mie));

  // === PMP SETTINGS FOR QEMU ===
  // Enable accsess to the all memory: addr = 2^XLEN - 1, A = NAPOT, R/W/X
  uint64_t pmp_addr = ~0UL >> 2;  // Все биты, кроме двух младших
  write_csr(pmpaddr0, pmp_addr);
  write_csr(pmpcfg0, 0xF); // A=NAPOT (0b11), R=1, W=1, X=1 => 0b1111 = 0xF
  printf("pmpcfg0 = 0x%lx, pmpaddr0 = 0x%lx\n", read_csr(pmpcfg0), read_csr(pmpaddr0));

  uint64_t mst = read_csr(mstatus);
  mst &= ~(3 << 11);      // clean MPP
  mst |= (1 << 11);       // MPP = S-mode (01)
  mst |= (1 << 18);       // SUM=1 (S-mode can read/write U-mode memory)
  mst |= (1 << 3);  

  write_csr(mstatus, mst);

  SET_MEPC_SAFE(smode_main);
  uint64_t satp = read_csr(satp);
  printf("satp = 0x%lx\n", satp);
  printf("enter ro S-mode: mepc=0x%lx, mstatus=0x%lx\n", read_csr(mepc), read_csr(mstatus));

  asm volatile ("mret");
}
// Called before main() to get the environment set up and sane.

void main(int argc, char* argv[], char* envp[]) {
  extern void panic(const char* msg) __attribute__((noreturn));
  extern void setup_mmu();
  
  static int premain_called;
  if (premain_called++) {
    panic("PANIC: _premain called multiple times");
  }

  // We have to zero our globals that aren't initialized.
  extern char _sbss; // provided by linker script.
  extern char _ebss;
  our_memset(&_sbss, 0, &_ebss - &_sbss);

  setup_mmu();
  enter_smode();

  //extern void rearm_timer();
  //rearm_timer();
}
