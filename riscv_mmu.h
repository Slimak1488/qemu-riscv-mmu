#include "platform.h"

#define PAGE_SIZE 4096
#define NULL ((void*)0) 
#define PTE_PAGE_OFFSET 12

#define TABLE_SIZE 512

struct Table
{
    uint64_t entries[TABLE_SIZE];
};

typedef struct Table table_t;

enum EntryBits {
	None = 0,
	Valid = 1 << 0,
	Read = 1 << 1,
	Write = 1 << 2,
	Execute = 1 << 3,
	User = 1 << 4,
	Global = 1 << 5,
	Access = 1 << 6,
	Dirty = 1 << 7,

	// Convenience combinations
	ReadWrite = 1 << 1 | 1 << 2,
	ReadExecute = 1 << 1 | 1 << 3,
	ReadWriteExecute = 1 << 1 | 1 << 2 | 1 << 3,

	// User Convenience Combinations
	UserReadWrite = 1 << 1 | 1 << 2 | 1 << 4,
	UserReadExecute = 1 << 1 | 1 << 3 | 1 << 4,
	UserReadWriteExecute = 1 << 1 | 1 << 2 | 1 << 3 | 1 << 4,
};

//enum EntryBits entryBits;

uint8_t is_leaf(const uint64_t *entry) {
  if (*entry & 0xe != 0) return 1;
  return 0;
}

uint8_t is_valid(const uint64_t *entry) {
  if (*entry & Valid) return 1;
  return 0;
}

//void set_entry
