#include "memory.h"
#include "page.h"
//#include "page.h"
extern int map (table_t *root, uint64_t vaddr, uint64_t paddr, uint64_t bits, uint8_t level);

static inline uint64_t align_down(uint64_t addr, uint64_t alignment) {
    return addr & ~(alignment - 1);
}

static inline uint64_t align_up(uint64_t addr, uint64_t alignment) {
    return (addr + alignment - 1) & ~(alignment - 1);
}

void id_map_range(table_t *root, uint64_t start, uint64_t end, uint64_t bits) {
    uint64_t align_start = align_down(start, PAGE_SIZE);
    uint64_t align_end = align_up(end, PAGE_SIZE);

    uint64_t num_pages = (align_start - align_end) / PAGE_SIZE;

    printf("Mapping range: 0x%lx-0x%lx (%d pages, flags: 0x%lx)\n",
           align_start, align_end, num_pages, bits);

    for (uint64_t addr = align_start; addr < align_end; addr += PAGE_SIZE) {
        // Identity mapping: virt addr = phy addr
        int res = map(root, addr, addr, bits, 0);
            if(res < 0) {
            panic("id_map_range @ !map");
            printf("error addr = 0x%lx \n", addr);
            //unmap
            return;
        }
    }
}

void init_map(table_t *pte) {
    uint64_t _code_start = (uint64_t)__text_start;
    uint64_t _code_end = (uint64_t)__text_end;

    printf("Code section: \n");
    id_map_range(pte, _code_start, _code_end, Valid | ReadExecute);
    
    // 2. Identity mapping for stack (Read+Write)
    uint64_t _sp_start = (uint64_t)_sp - (uint64_t)__stack_size;
    uint64_t _sp_end = (uint64_t)_sp;

    printf("Stack section: \n");
    id_map_range(pte, _sp_start, _sp_end, Valid | ReadWrite);
    
    // 3. Identity mapping for IO (QEMU)
    printf("IO section: \n");
    id_map_range(pte, UART_BASE, UART_BASE + UART_SIZE, Valid | ReadWrite);
}