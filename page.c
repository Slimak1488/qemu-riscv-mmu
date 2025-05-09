#include "page.h"

table_t* root_pte = NULL;

extern void *aligned_alloc(uint64_t align, uint64_t size);

table_t* zalloc_pages(int pages) {
  if (pages == 0) {
        panic("alloc_pages: pages == 0\n");
        return NULL;
    }

    void* ptr = aligned_alloc(PAGE_SIZE, pages * PAGE_SIZE);

    if (!ptr) {
        panic("alloc_pages: aligned_alloc failed");
        return NULL;
    }

    uint64_t* p = (uint64_t*)ptr;
    uint64_t n = (pages * PAGE_SIZE) / sizeof(uint64_t);

    for (uint64_t i = 0; i < n; i++) {
        p[i] = 0;
    }

    return ptr;
}

uint8_t is_leaf(const uint64_t *entry) {
  if (*entry & ReadWriteExecute != 0) return 1;
  return 0;
}

uint8_t is_valid(const uint64_t *entry) {
  if (*entry & Valid) return 1;
  return 0;
}

table_t* init_page() {
    root_pte = zalloc_pages(1);

    if (!root_pte) {
        panic("!init_page");
        return NULL;
    }

    printf("root_pte=0x%lx\n", root_pte);

    // Check aligned (0 in the lower 12 bits)
    if ((uint64_t)root_pte & 0xFFF) {
        printf("Root page table misaligned: 0x%lx\n", (uint64_t)root_pte);
        panic("!init_page");
        return NULL;
    }

    return root_pte;
}