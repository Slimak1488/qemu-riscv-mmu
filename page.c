#include "page.h"

table_t* root_pte = NULL;

extern void *zsimple_aligned_alloc(uint64_t align, uint64_t size);


//table_t* page_alloc(int n) {
//
//}
uint8_t is_leaf(const uint64_t *entry) {
  if (*entry & ReadWriteExecute != 0) return 1;
  return 0;
}

uint8_t is_valid(const uint64_t *entry) {
  if (*entry & Valid) return 1;
  return 0;
}

table_t* init_page() {

    root_pte = (table_t*)zsimple_aligned_alloc(PAGE_SIZE, PAGE_SIZE);

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