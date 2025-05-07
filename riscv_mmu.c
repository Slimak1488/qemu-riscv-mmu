#include "riscv_encoding.h"
#include "page.h"

extern panic(const char* msg);
extern puts(char* message);
extern zsimple_aligned_alloc(uint64_t align, uint64_t size);
extern uint8_t is_leaf(const uint64_t *entry);
extern uint8_t is_valid(const uint64_t *entry);

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

    if(!root) return -2;
    
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
            pte_t *page = (pte_t*)zsimple_aligned_alloc(PAGE_SIZE, 1); //entry

            if (!page) return -3;

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
extern table_t *root_pte;
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

    uint64_t flags = Valid | User;  // Valid + User
    switch (cause) {
        case 12:  // Load page fault
            flags |= Read;
            break;
        case 13:  // Store page fault
            flags |= ReadWrite;
            break;
        case 15:  // Instruction page fault
            flags |= Execute;
            break;
    }

    printf("PAGE FAULT @ aligned_vaddr=0x%lx \n", vaddr_page_fault);
    
    if (!root_pte) {
        panic("PAGE FAULT @ root_pte is NULL");
        return;
    }

    int res = map(root_pte, vaddr_page_fault, (uint64_t)phy_page, flags, 0);

    if (res < 0) {
        printf("Error: res=%d\n", res);
        panic("PAGE FAULT @ !map");
        //unmap
        return;
    }

    asm volatile ("sfence.vma %0" : : "r" (vaddr_page_fault));

    printf("Mapped: VA 0x%lx -> PA 0x%lx, flags=0x%lx\n", 
       vaddr_page_fault, phy_page, flags);
}

void* setup_mmu(table_t *root) {

    if (!root) {
        panic("!setup_mmu");
        return NULL;
    }

    /*
    IT: 63      60 59                44 43                    0
        +--------+---------------------+-----------------------+
        | MODE=8 |       ASID=0        |    PPN (root_pte>>12) |
        +--------+---------------------+-----------------------+
       The physical address of the page table must be 4KB aligned (low 12 bits = 0)
        A 12 bit shift removes these zero bits, leaving only the physical page number (PPN). 
    */

    // SATP
    uint64_t root_ppn = ((uint64_t)root) >> 12;  // Physical address without the lower 12 bits
    uint64_t satp_value = (8ULL << 60) | root_ppn;   // MODE=8 (Sv39), ASID=0

    printf("Root PTE phys = 0x%lx\n", root_ppn << 12);
    printf("SATP value    = 0x%lx\n", satp_value);

    write_csr(satp, satp_value);
    asm volatile ("sfence.vma zero, zero");
}