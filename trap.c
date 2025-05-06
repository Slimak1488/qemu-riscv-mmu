
#include "riscv_encoding.h"

//void trap (void) __attribute__ ((interrupt ("machine")));
#pragma GCC optimize ("align-functions=4")

extern rearm_timer(void);
extern page_fault(int cause);
extern void printf(const char* fmt, ...);

long interrupt_count;

// if the reason for the panic() is a blown stack pointer, this panic
// -- indeed, any written in C -- is going to have a baad time. This also
// means that $a0 is clobbered (by msg) and the system may otherwise be
// perturbed.
void panic(const char* msg) {
  puts(msg);
  putc('\n');
}

static interrupt(int intr) {
  switch(intr) {
    case IRQ_M_TIMER: // Machine Mode Timer
      rearm_timer();
      interrupt_count++;
      puts(" T");
      break;
    default:
      panic("Unexpected synchronous exception.");
  }
}

static exception(int cause) {
  // Handle types as listed in DECLARE_CAUSE.
  switch (cause)
  {
    case 12: case 13: case 14: case 15:
      printf("PAGE FAULT @ VA(mtval)=0x%lx, PC(mepc)=0x%lx\n", read_csr(mtval), read_csr(mepc));
      page_fault(cause);
      break;
    case 7:
      break;
    default:
      printf("DEFAULT CASE @ VA(mtval)=0x%lx, PC(mepc)=0x%lx\n", read_csr(mtval), read_csr(mepc));
      break;
  }
}

void trap(void) {
  long long cause = read_csr(mcause);
  printf("Trap. cause = %d (0x%lx) \n", cause, cause);

  // MSTATUS_SD is always the top bit in the (32/64 bit) word. _SD is set
  // via preprocessing voodoo in riscv_encoding.h.
  
  if (cause & MSTATUS_SD) {
    interrupt(cause & 0xff);
  } else {
    exception(cause & 0xffff);
  }

  asm volatile("mret");
}

const char past_main[] = "PANIC: fell off the end of main()";