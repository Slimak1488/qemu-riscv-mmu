#include "riscv_encoding.h"
#include "riscv_mmu.h"

extern panic(const char* msg);
extern puts(char* message);

extern uint64_t __heap_start[];
extern uint64_t __heap_end[];

static uint64_t *cur_heap_ptr = NULL;

table_t* root_pte = NULL;

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

void *zsimple_aligned_alloc(uint64_t align, uint64_t size) {
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

    for (uint64_t i = 0; i < PAGE_SIZE/sizeof(table_t); i++) align_ptr[i] = 0;

    return (void*) align_ptr;
}

/*
    В режиме Sv39 (для 64-битных систем) виртуальный адрес имеет следующую структуру:

    Copy
    63         39      30      21      12         0
    +---------+-------+-------+-------+-----------+
    |  Sign   | VPN[2]| VPN[1]| VPN[0]| Page Offset|
    |  Ext    | (9 бит)| (9 бит)| (9 бит)| (12 бит)  |
    +---------+-------+-------+-------+-----------+
                       12 offt 0
    virt 0xFFFF FFFF C0|00 1000|
    phy                
*/ 

 /*
    Каждая PTE (Page Table Entry) содержит:

    63      54 53    28 27    10 9   8 7 6 5 4 3 2 1 0
    +--------+--------+--------+---+---+---+---+---+---+
    | Reserved| PPN[2] | PPN[1] |PPN0| RSW |D|A|G|U|X|W|R|V|
    +--------+--------+--------+---+---+---+---+---+---+
    V -> RSW = 10bit
*/

/// Map a virtual address to a physical address using 4096-byte page size.
/// root: reference to the root Table
/// vaddr: The virtual address to map
/// paddr: The physical address to map
/// bits: An OR'd bitset containing the bits the leaf should have.
///       The bits should contain only the following:
///          Read, Write, Execute, User, and/or Global
///       The bits MUST include one or more of the following:
///          Read, Write, Execute
///       The valid bit automatically gets added.
int map(table_t *root, uint64_t vaddr, uint64_t paddr, uint64_t bits, uint8_t level) {
    if (!(bits & 0xe)) return -1;
    
    uint64_t vpn[3] = {
        vaddr >> 12 & 0x1ff, 				// VPN[0] = vaddr[20:12]
        vaddr >> 21 & 0x1ff,				// VPN[1] = vaddr[29:21]
        vaddr >> 30 & 0x1ff 				// VPN[2] = vaddr[38:30]
    };

    uint64_t ppn[3] = {
        paddr >> 12 & 0x1ff, 				// PPN[0] = paddr[20:12]
        paddr >> 21 & 0x1ff,				// PPN[1] = paddr[29:21]
        paddr >> 30 & 0x3ffffff 			// PPN[2] = paddr[55:30]
    };

    uint64_t *v = &root->entries[vpn[2]];

    for (int8_t i = 1; i >= level; i--) {
        printf("map @ level=%d, v=%lx\n", i, v);
        if (!is_valid(v)) {
            pte_t *page = (pte_t*)zsimple_aligned_alloc(PAGE_SIZE, 1);

            if (!page) return -1;

            *v = ((uint64_t)page >> 2) | Valid;
        }

        uint64_t entry_value = (*v & ~0x3ff) << 2; // next level ((>> PTE_PAGE_OFFSET) << PTE_PPN_SHIFT) = 12 - 10 = 2)
        uint64_t *entry = (uint64_t*)entry_value;
        v = &entry[vpn[i]];
    }

    uint64_t entry = (ppn[2] << 28) |   // PPN[2] = [53:28]
	            (ppn[1] << 19) |        // PPN[1] = [27:19]
				(ppn[0] << 10) |        // PPN[0] = [18:10]
				bits  |                 // Specified bits, such as User, Read, Write, etc
				Valid |                 // Valid bit
				Dirty |                 // Some machines require this to =1
				Access                  // Just like dirty, some machines require this
				;
	// Set the entry. V should be set to the correct pointer by the loop
	// above.
	*v = entry;
    printf("end map @ entry=0x%lx\n", v);

    return 0;
}

/// Unmaps and frees all memory associated with a table.
/// root: The root table to start freeing.
/// NOTE: This does NOT free root directly. This must be
/// freed manually.
/// The reason we don't free the root is because it is
/// usually embedded into the Process structure.
//void unmap(table_t *root) {
//    for (uint8_t lvl2 = 0; lvl2 < 2; ++lvl2) {
//        uint64_t entry_lvl2 = root->entries[lvl2];
//        if (is_valid(&entry_lvl2) && !is_leaf(&entry_lvl2)) {
//            uint64_t memaddr_lvl1 = (entry_lvl2 & ~0x3FF) << 2;
//            table_t *table_lvl1 = &memaddr_lvl1;
//            for (uint64_t lvl1 = 0; lvl1 < 2; ++lvl1) {
//                uint64_t entry_lvl1 = table_lvl1->entries[lvl1];
//                if (is_valid(&entry_lvl1) && !is_leaf(&entry_lvl1)) {
//                    uint64_t memaddr_lvl0 = (entry_lvl1 & ~0x3ff) << 2;
//                    // The next level is level 0, which
//					// cannot have branches, therefore,
//					// we free here.
//
//                    //dealloc(memaddr_lvl0);
//                }
//                //dealloc(memaddr_lvl1);
//
//            }
//        }
//    }
//}

void* virt_to_phy(table_t *root, uint64_t vaddr) {
    uint64_t vpn[3] = {
        vaddr >> 12 & 0x1ff, 				// VPN[0] = vaddr[20:12]
        vaddr >> 21 & 0x1ff,				// VPN[1] = vaddr[29:21]
        vaddr >> 30 & 0x1ff 				// VPN[2] = vaddr[38:30]
    };
    
    uint64_t *v = &root->entries[vpn[2]];
    for (uint8_t i = 2; i >= 0; i-- ) {
        if (!is_valid(v)) {
            // This is an invalid entry, page fault.
            break;
        } else if(is_leaf(v)) {
            uint64_t off_mask = (1 << (PTE_PAGE_OFFSET + i * 9)) - 1;
			uint64_t vaddr_pgoff = vaddr & off_mask;
            uint64_t addr = (*v << 2) & ~off_mask;
        }

    }

    return NULL;
}

void page_fault(int cause) {
    uint64_t vaddr_page_fault = read_csr(mtval);
    printf("PAGE FAULT @ vaddr=0x%lx \n", vaddr_page_fault);

    vaddr_page_fault &= ~(PAGE_SIZE - 1);
    uint64_t *phy_page = (uint64_t*)zsimple_aligned_alloc(PAGE_SIZE, 1);

    if (!phy_page) {
        panic("!phy_page");
        return;
    }
    else {
        printf("phy_page = 0x%lx \n", phy_page);
    }

    uint64_t flags = PTE_V | PTE_U;  // Valid + User
    switch (cause) {
        case 12:  // Load page fault
            flags |= PTE_R;
            break;
        case 13:  // Store page fault
            flags |= PTE_R | PTE_W;
            break;
        case 15:  // Instruction page fault
            flags |= PTE_X;
            break;
    }

    printf("PAGE FAULT @ aligned_vaddr=0x%lx \n", vaddr_page_fault);
   
    int res = map(root_pte, vaddr_page_fault, (uint64_t)phy_page, flags, 0);
    if (res < 0) {
        panic("PAGE FAULT @ !map");
        //unmap
        return;
    }

    asm volatile ("sfence.vma %0" : : "r" (vaddr_page_fault));

    printf("Mapped: VA 0x%lx -> PA 0x%lx, flags=0x%lx\n", 
       vaddr_page_fault, phy_page, flags);
}

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

void setup_mmu() {
    root_pte = (table_t*)zsimple_aligned_alloc(PAGE_SIZE, PAGE_SIZE);

    if (!root_pte) {
        panic("!setup_mmu");
        return;
    }
    printf("root_pte=0x%lx \n", root_pte);

    /*
    IT: 63      60 59                44 43                    0
        +--------+---------------------+-----------------------+
        | MODE=8 |       ASID=0        |    PPN (root_pte>>12) |
        +--------+---------------------+-----------------------+
       The physical address of the page table must be 4KB aligned (low 12 bits = 0)
        A 12 bit shift removes these zero bits, leaving only the physical page number (PPN). 
    */

    // 1. Identity mapping for code section (Read+Execute)
    #define CODE_START 0x80000000
    #define CODE_END   0x80020000
    extern uint64_t __text_start[];
    extern uint64_t __text_end[];
    printf("Code section: \n");
    id_map_range(root_pte, (uint64_t)__text_start, (uint64_t)__text_end, PTE_V | PTE_R | PTE_X);
    
    // 2. Identity mapping for stack (Read+Write)
    #define STACK_BASE 0x82000000
    #define STACK_SIZE (4 * 1024) // 64KB
    extern uint64_t _sp[];
    extern uint64_t __stack_size[];
    uint64_t _sp_start = (uint64_t)_sp - (uint64_t)__stack_size;
    uint64_t _sp_end = (uint64_t)_sp;
    printf("Stack section: \n");
    id_map_range(root_pte, _sp_start, _sp_end, PTE_V | PTE_R | PTE_W);
    
    // 3. Identity mapping for IO (QEMU)
    #define UART_BASE 0x10000000
    #define UART_SIZE 0x1000
    printf("IO section: \n");
    id_map_range(root_pte, UART_BASE, UART_BASE + UART_SIZE, PTE_V | PTE_R | PTE_W);


    // SATP
    uint64_t root_ppn = ((uint64_t)root_pte) >> 12;  // Physical address without the lower 12 bits
    uint64_t satp_value = (8ULL << 60) | root_ppn;   // MODE=8 (Sv39), ASID=0

    // Check aligned (0 in the lower 12 bits)
    if ((uint64_t)root_pte & 0xFFF) {
        printf("Root page table misaligned: 0x%lx", (uint64_t)root_pte);
        return;
    }

    printf("Root PTE phys = 0x%lx\n", root_ppn << 12);
    printf("SATP value    = 0x%lx\n", satp_value);

    write_csr(satp, satp_value);
    asm volatile ("sfence.vma zero, zero");
}