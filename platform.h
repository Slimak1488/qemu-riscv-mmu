#if ( __riscv_xlen == 64)
  typedef unsigned long uint64_t;
  typedef long uint32_t;
#else
  typedef unsigned long long uint64_t;
  typedef long uint32_t;
#endif

#define NULL ((void*)0) 
#define VIRT_UART 0x10000000

typedef unsigned char uint8_t;
typedef signed char int8_t;