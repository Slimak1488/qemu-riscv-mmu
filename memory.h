#include "platform.h"

#define UART_BASE VIRT_UART
#define UART_SIZE 0x1000

extern uint64_t __text_start[];
extern uint64_t __text_end[];

extern uint64_t _sp[];
extern uint64_t __stack_size[];

extern uint64_t __heap_start[];
extern uint64_t __heap_end[];