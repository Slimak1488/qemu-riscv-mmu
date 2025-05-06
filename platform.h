#if ( __riscv_xlen == 64)
  typedef unsigned long uint64_t;
  typedef long uint32_t;
#else
  typedef unsigned long long uint64_t;
  typedef long uint32_t;
#endif
typedef uint64_t pte_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;