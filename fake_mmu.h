#pragma once
#include <stdint.h>
#include <stdio.h>

#define PHYSICAL_MEMORY_SIZE (1 << 20) // 1 MB
#define VIRTUAL_MEMORY_SIZE (1 << 24)  // 16 MB
#define PAGE_SIZE (1 << 12)            // 4 KB
#define NUM_PAGES (VIRTUAL_MEMORY_SIZE / PAGE_SIZE)
#define NUM_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)
#define NUM_SEGMENTS 16
#define PAGE_NBITS 12
#define FRAME_NBITS 12
#define SEGMENT_FLAGS_NBITS 3

#define PAGE_FLAGS_NBITS 5

typedef enum {
    Valid = 0x1,
    Read = 0x2,
    Write = 0x4
} SegmentFlags;

typedef enum {
    PageValid = 0x1,
    ReadBit = 0x2,
    WriteBit = 0x4,
    Unswappable = 0x8
} PageFlags;

typedef struct SegmentDescriptor {
    uint32_t base: PAGE_NBITS;
    uint32_t limit: 24;
    SegmentFlags flags: SEGMENT_FLAGS_NBITS;
} SegmentDescriptor;

typedef struct PageEntry {
    uint32_t frame_number: PAGE_NBITS;
    uint32_t flags: PAGE_FLAGS_NBITS;
} PageEntry;

typedef struct MMU {
    PageEntry* pages;
    SegmentDescriptor* segments;
    char physical_memory[PHYSICAL_MEMORY_SIZE];
    FILE* swap_file;
    int page_fault_count;
    int num_pages;
    int num_segments;
} MMU;

typedef struct LogicalAddress {
    uint32_t segment_id;
    uint32_t page_number;
    uint32_t offset;
} LogicalAddress;

typedef struct LinearAddress {
    uint32_t page_number;
    uint32_t offset;
} LinearAddress;

typedef uint32_t PhysicalAddress;

MMU* MMU_init(const char* swap_filename);
void MMU_free(MMU* mmu);
void MMU_exception(MMU* mmu, uint32_t page_number);
LinearAddress getLinearAddress(MMU* mmu, LogicalAddress logical_address);
uint32_t getPhysicalAddress(MMU* mmu, LinearAddress linear_address);
void MMU_writeByte(MMU* mmu, LogicalAddress logical_address, char c);
char MMU_readByte(MMU* mmu, LogicalAddress logical_address);
int find_free_frame_or_replace(MMU* mmu);
void MMU_exportToCSV(MMU* mmu, const char* filename);