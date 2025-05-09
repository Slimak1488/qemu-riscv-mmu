#include "memory.h"

static uint64_t *cur_heap_ptr = NULL;

void *simple_malloc(uint64_t size) {
    if (cur_heap_ptr == NULL) {
        cur_heap_ptr = __heap_start;
    }

    if (size == 0) return NULL;

    if (cur_heap_ptr + size > __heap_end) {
        return NULL;
    }
    uint64_t aligned_size = (size + 7) & ~7;
    uint64_t *ptr = cur_heap_ptr;
    cur_heap_ptr += size;

    return (void*)ptr;
}

//void dealloc(uint64_t *ptr) {
//    if (ptr == NULL) {
//        printf("dealloc @ ptr is NULL \n");
//        return;
//    }
//
//    uint64_t addr = __heap_start;
//
//}

void *aligned_alloc(uint64_t align, uint64_t size) {
    if (cur_heap_ptr == NULL) {
        cur_heap_ptr = __heap_start;
    }

    if (size == 0 || align == 0) {
        panic("simple_aligned_alloc: size == 0 || align == 0");
        return NULL;
    }

    if ((align & (align - 1)) != 0) { // (8)1000 & (7)0111 != 0
        panic("simple_aligned_alloc: (align & (align - 1)) != 0");
        return NULL;
    }

    uint64_t *addr = cur_heap_ptr;
    uint64_t *align_ptr = (uint64_t*)((uint64_t)(addr + align - 1) & ~(align - 1));

    uint64_t *new_ptr = align_ptr + size;

    if (new_ptr > (uint64_t*)__heap_end) {
        printf("new_ptr=%lx heap_end=%lx \n", new_ptr, (uint64_t*)__heap_end);
        panic("simple_aligned_alloc: new_ptr > (uint64_t*)__heap_end");
        return NULL;
    }

    cur_heap_ptr = new_ptr;

    //for (uint64_t i = 0; i < PAGE_SIZE/sizeof(table_t); i++) align_ptr[i] = 0;

    return (void*) align_ptr;
}