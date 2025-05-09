// Platform-specific code. Could be used to shim printing to a UART or
// semihosting or something.
// This is for QEMU -machine virt -bios none -nographic

#include "riscv_encoding.h"
#include "platform.h"
#include "page.h"

#define MSTATUS_MPP_S (1 << 11)  // MPP=1 (Supervisor)
#define MSTATUS_SUM   (1 << 18)  // SUM=1
#define MSTATUS_CLEAN_MPP ~(3 << 11)

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

extern void panic(const char* msg) __attribute__((noreturn));

extern void setup_mmu(table_t *root);
extern table_t* init_page();
extern void init_map(table_t *pte); 

extern uint64_t main();

void putc(char c) {
    *(volatile char*) VIRT_UART = c;
}

void puts(char* message) {
  while (*message) {
    putc(*message++);
  }
}

void* our_memset(void* start, int data, int len) {
  unsigned char* p = start;
  while(len--) {
    *p++ = (unsigned char)data;
  }
  return start;
}

void enter_smode() {
  extern void trap(void);

  write_csr(mtvec, trap);
  printf("mtvec: 0x%lx \n", read_csr(mtvec));

  write_csr(mie, (1 << IRQ_HOST) | (1 << IRQ_COP) | MIP_SEIP | MIP_STIP | MIP_MSIP);
  printf("mie: 0x%lx \n", read_csr(mie));

  // === PMP SETTINGS FOR QEMU ===
  // Enable accsess to the all memory: addr = 2^XLEN - 1, A = NAPOT, R/W/X
  uint64_t pmp_addr = ~0UL >> 2;  // All bits except the two low bits
  write_csr(pmpaddr0, pmp_addr);
  write_csr(pmpcfg0, 0xF); // A=NAPOT (0b11), R=1, W=1, X=1 => 0b1111 = 0xF
  printf("pmpcfg0 = 0x%lx, pmpaddr0 = 0x%lx\n", read_csr(pmpcfg0), read_csr(pmpaddr0));

  uint64_t mst = read_csr(mstatus);
  mst &= MSTATUS_CLEAN_MPP;      // clean MPP
  mst |= MSTATUS_MPP_S;   // MPP = S-mode (01)
  mst |= MSTATUS_SUM;     // SUM=1 (S-mode can read/write U-mode memory)
  mst |= MSTATUS_MIE;     

  write_csr(mstatus, mst);

  SET_MEPC_SAFE(main);

  printf("satp = 0x%lx\n", read_csr(satp));

  printf("enter to S-mode: mepc=0x%lx, mstatus=0x%lx\n",
                       read_csr(mepc), read_csr(mstatus));

  asm volatile ("mret");
}
// Called before main() to get the environment set up and sane.

void platform_init() {
  

  static int premain_called;
  if (premain_called++) {
    panic("PANIC: _premain called multiple times");
  }

  // We have to zero our globals that aren't initialized.
  extern char _sbss; // provided by linker script.
  extern char _ebss;
  our_memset(&_sbss, 0, &_ebss - &_sbss);
  
  table_t *p_root = init_page();

  if (!p_root) {
    panic("root table is NULL");
    return;
  }

  init_map(p_root);
  setup_mmu(p_root);

  enter_smode();

  //extern void rearm_timer();
  //rearm_timer();
}
