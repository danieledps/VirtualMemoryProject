#include "fake_mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_MMU_initialization() {
    printf("Running test: MMU_initialization...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    for (int i = 0; i < mmu->num_segments; i++) {
        assert(mmu->segments[i].flags & Valid && "Segment should be initialized as valid");
    }
    for (int i = 0; i < mmu->num_pages; i++) {
        assert(!(mmu->pages[i].flags & PageValid) && "Page should be initially invalid");
    }
    MMU_free(mmu);
    printf("Test MMU_initialization passed.\n");
}

void test_page_fault_handling() {
    printf("Running test: Page Fault Handling...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    LogicalAddress la = {0, 0, 0};
    char data = MMU_readByte(mmu, la);
    printf("Read data after page fault: %c\n", data);
    assert(mmu->pages[0].flags & PageValid && "Page should be valid after page fault");
    MMU_free(mmu);
    printf("Test Page Fault Handling passed.\n");
}

void test_write_and_read() {
    printf("Running test: Write and Read...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    LogicalAddress la = {0, 1, 100};
    MMU_writeByte(mmu, la, 'A');
    assert(mmu->pages[1].flags & WriteBit && "Page should have WriteBit set after write");
    assert(mmu->pages[1].flags & ReadBit && "Page should have ReadBit set after write");
    char c = MMU_readByte(mmu, la);
    assert(c == 'A' && "The byte read should match the written byte");
    MMU_free(mmu);
    printf("Test Write and Read passed.\n");
}

void test_second_chance_algorithm() {
    printf("Running test: Second Chance Algorithm...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    for (int i = 0; i < NUM_FRAMES; i++) {
        LogicalAddress la = {0, i, 0};
        MMU_writeByte(mmu, la, 'B' + i);
    }
    LogicalAddress la = {1, 0, 0};
    MMU_writeByte(mmu, la, 'B' + 256);
    assert(!(mmu->pages[0].flags & PageValid) && "Page 0 should have been replaced.");
    MMU_free(mmu);
    printf("Test Second Chance Algorithm passed.\n");
}

void test_access_out_of_segment() {
    printf("Running test: Access Out Of Segment...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    LogicalAddress la = {0, 257, 0};
    printf("Segment ID: %d, Page Number: %d, Segment Limit: %d\n", la.segment_id, la.page_number, mmu->segments[la.segment_id].limit);
    assert(!(la.page_number < mmu->segments[la.segment_id].limit) && "Access should fail due to out of bounds");
    MMU_free(mmu);
    printf("Test Access Out Of Segment passed.\n");
}

int main() {
    test_MMU_initialization();
    test_page_fault_handling();
    test_write_and_read();
    test_second_chance_algorithm();
    test_access_out_of_segment();
    printf("All tests passed!\n");
    return 0;
}
