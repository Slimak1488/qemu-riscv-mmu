
#include "riscv_encoding.h"

// This isn't really simpler than doing it in the startup code. It's just
// an example/reminder that you have no libc and tending your startup really
// is your own responsibility.
// This is also so we have an "interesting" loop to look at in the generated
// assembly.

void* our_memset(void* start, int data, int len) {
  unsigned char* p = start;
  while(len--) {
    *p++ = (unsigned char)data;
  }
  return start;
}

static int foreground_count;
#if ( __riscv_xlen == 64)
  typedef unsigned long uint64_t;
  typedef long uint32_t;
#else
  typedef unsigned long long uint64_t;
  typedef long uint32_t;
#endif
typedef uint64_t pte_t;

extern void printf(const char* fmt, ...);

__attribute__((aligned(4), noreturn))
uint64_t smode_main() {
  printf("enter to smod_main \n");
  
  // check MMU
  uint64_t satp = read_csr(satp);
  if ((satp >> 60) == 0) {
      printf("MMU disabled (satp.MODE = Bare)\n");
  } else {
      printf("MMU enabled (satp=0x%lx)\n", satp);
  }
  
  volatile uint64_t* test_ptr1 = (volatile uint64_t*)0x90003000;
  printf("Value of 0x90003000 before writing: 0x%d\n", *test_ptr1);  
  *test_ptr1 = 42;
  printf("Value of 0x90003000 after writing: 0x%d\n", *test_ptr1);  


  volatile uint64_t* test_ptr2 = (volatile uint64_t*)0xC0000000;
  printf("Value of 0xC0000000 before writing: 0x%d\n", *test_ptr2);  
  *test_ptr2 = 43;
  printf("Value of 0xC0000000 after writing: 0x%d\n", *test_ptr2);

  while (1) {
      asm volatile("wfi");
  }
}