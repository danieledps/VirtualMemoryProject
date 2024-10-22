#include "fake_mmu.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

MMU* MMU_init(const char* swap_filename) {
    MMU* mmu = (MMU*)malloc(sizeof(MMU));
    assert(mmu != NULL && "Memory allocation for MMU failed");
    
    mmu->pages = (PageEntry*)malloc(NUM_PAGES * sizeof(PageEntry));
    assert(mmu->pages != NULL && "Memory allocation for pages failed");
    
    mmu->segments = (SegmentDescriptor*)malloc(NUM_SEGMENTS * sizeof(SegmentDescriptor));
    assert(mmu->segments != NULL && "Memory allocation for segments failed");
    
    mmu->frame_usage = (int*)calloc(NUM_FRAMES, sizeof(int));
    assert(mmu->frame_usage != NULL && "Memory allocation for frame usage failed");

    for (int i = 0; i < NUM_PAGES; i++) {
        mmu->pages[i].frame_number = 0;  
        mmu->pages[i].flags = 0;         
    }

    for (int i = 0; i < NUM_SEGMENTS; i++) {
        mmu->segments[i].base = i * (NUM_PAGES / NUM_SEGMENTS); 
        mmu->segments[i].limit = NUM_PAGES / NUM_SEGMENTS;      
        mmu->segments[i].flags = Valid;                                   
    }

    mmu->swap_file = fopen(swap_filename, "r+b");
    if (mmu->swap_file == NULL) {
        mmu->swap_file = fopen(swap_filename, "wb+");
        assert(mmu->swap_file != NULL && "Error opening file swap");
        char* buffer = (char*)calloc(VIRTUAL_MEMORY_SIZE, sizeof(char));
        assert(buffer != NULL && "Error allocating buffer for swap file initialization");
        fwrite(buffer, sizeof(char), VIRTUAL_MEMORY_SIZE, mmu->swap_file); 
        free(buffer);
        rewind(mmu->swap_file);
    }

    mmu->num_pages = NUM_PAGES;
    mmu->num_segments = NUM_SEGMENTS;
    mmu->page_fault_count = 0;

    return mmu;
}


void MMU_free(MMU* mmu) {
    fclose(mmu->swap_file);
    free(mmu->pages);
    free(mmu->segments);
    free(mmu->frame_usage);
    free(mmu);
}

LinearAddress getLinearAddress(MMU* mmu, LogicalAddress logical_address) {
    assert(logical_address.segment_id < mmu->num_segments && "segment out of bounds");
    SegmentDescriptor descriptor = mmu->segments[logical_address.segment_id];
    assert(descriptor.flags & Valid && "invalid_segment");
    assert(logical_address.page_number < descriptor.limit && "out_of_segment_limit");

    LinearAddress linear_address;
    linear_address.page_number = descriptor.base + logical_address.page_number;
    linear_address.offset = logical_address.offset;
    return linear_address;
}

uint32_t getPhysicalAddress(MMU* mmu, LinearAddress linear_address) {
    PageEntry page_entry = mmu->pages[linear_address.page_number];
    assert(page_entry.flags & PageValid && "invalid page");
    uint32_t frame_number = page_entry.frame_number;
    return (frame_number << FRAME_NBITS) | linear_address.offset;
}

void MMU_exception(MMU* mmu, uint32_t page_number) {
    assert(page_number < mmu->num_pages && "page_number out of bounds");
    mmu->page_fault_count++;
    int frame_number = find_free_frame_or_replace(mmu);
    if (mmu->pages[frame_number].flags & PageValid) {
        fseek(mmu->swap_file, mmu->pages[frame_number].frame_number * PAGE_SIZE, SEEK_SET);
        size_t write_size = fwrite(&mmu->physical_memory[frame_number * PAGE_SIZE], PAGE_SIZE, 1, mmu->swap_file);
        assert(write_size == 1 && "Error writing to swap file during page fault");
        mmu->pages[frame_number].flags &= ~PageValid;
    }
    fseek(mmu->swap_file, page_number * PAGE_SIZE, SEEK_SET);
    size_t read_size = fread(&mmu->physical_memory[frame_number * PAGE_SIZE], PAGE_SIZE, 1, mmu->swap_file);
    assert(read_size == 1 && "Error reading from swap file during page fault");
    mmu->pages[page_number].frame_number = frame_number;
    mmu->pages[page_number].flags |= PageValid; 
}

void print_MMU_state(MMU* mmu) {
    printf("\n=== MMU State ===\n");

    printf("\nSegments:\n");
    printf("%-10s %-10s %-10s %-10s\n", "SegmentID", "Base", "Limit", "Flags");
    for (int i = 0; i < mmu->num_segments; i++) {
        SegmentDescriptor* seg = &mmu->segments[i];
        printf("%-10d %-10u %-10u %-10x\n", i, seg->base, seg->limit, seg->flags);
    }

    printf("\nPage Table:\n");
    printf("%-10s %-10s %-10s\n", "Page#", "Frame#", "Flags");
    for (int i = 0; i < mmu->num_pages; i++) {
        PageEntry* page = &mmu->pages[i];
        printf("%-10d %-10u %-10x\n", i, page->frame_number, page->flags);
    }

    printf("\nFrame Usage:\n");
    for (int i = 0; i < NUM_FRAMES; i++) {
        printf("Frame %d: %s\n", i, mmu->frame_usage[i] ? "Occupied" : "Free");
    }

    printf("\nSwap File Status: Page faults handled: %d\n", mmu->page_fault_count);

    printf("================\n");
}

int find_free_frame_or_replace(MMU* mmu) {
    static int next_frame = 0;
    int attempts = 0;

    while (attempts < NUM_FRAMES) { 
        PageEntry* page = &mmu->pages[next_frame];

        if (page->flags & Unswappable) {
            printf("Frame %d is unswappable, skipping.\n", next_frame);
            next_frame = (next_frame + 1) % NUM_FRAMES;
            attempts++;
            continue;
        }

        if (page->flags & ReadBit) {
            printf("Frame %d has ReadBit set. Resetting ReadBit.\n", next_frame);
            page->flags &= ~ReadBit;
            next_frame = (next_frame + 1) % NUM_FRAMES;
            attempts++;
        } else {
            printf("Frame %d found for replacement.\n", next_frame);
            return next_frame;
        }
    }

    fprintf(stderr, "No frame available for replacement. Check page states.\n");
    exit(-1);
}

void MMU_writeByte(MMU* mmu, LogicalAddress logical_address, char c) {
    LinearAddress linear_address = getLinearAddress(mmu, logical_address);
    if (!(mmu->pages[linear_address.page_number].flags & PageValid)) {
        printf("Page fault during write at page %u. Invoking MMU_exception.\n", logical_address.page_number);
        MMU_exception(mmu, linear_address.page_number);
    }
    PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
    mmu->physical_memory[physical_address] = c;
    mmu->pages[linear_address.page_number].flags |= WriteBit | ReadBit;
    printf("Written byte '%c' at page %u, offset %u (physical address: %u)\n", c, logical_address.page_number, logical_address.offset, physical_address);
}

char MMU_readByte(MMU* mmu, LogicalAddress logical_address) {
    LinearAddress linear_address = getLinearAddress(mmu, logical_address);
    if (!(mmu->pages[linear_address.page_number].flags & PageValid)) {
        printf("Page fault during read at page %u. Invoking MMU_exception.\n", logical_address.page_number);
        MMU_exception(mmu, linear_address.page_number); 
    }
    PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
    mmu->pages[linear_address.page_number].flags |= ReadBit;
    printf("Read byte '%c' from page %u, offset %u (physical address: %u)\n", mmu->physical_memory[physical_address], logical_address.page_number, logical_address.offset, physical_address);
    return mmu->physical_memory[physical_address];
}