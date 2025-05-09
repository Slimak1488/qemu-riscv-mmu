#pragma once
#include "platform.h"

typedef struct {
    uint64_t regs[32];       // 0-255 
    uint64_t fregs[32];      // 256-511 
    uint64_t satp;           // 512-519 
    uint8_t* trap_stack;     // 520-527 
    uint64_t hartid;         // 528-535 
} TrapFrame;