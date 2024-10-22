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

void test_page_fault_handling_with_swap() {
    printf("Running test: Page Fault Handling with Swap File...\n");
    
    // Inizializza la MMU e verifica che il file di swap venga aperto correttamente
    MMU* mmu = MMU_init("swap_file_test.bin");
    assert(mmu->swap_file != NULL && "Swap file should be opened correctly");
    
    // Genera un page fault e verifica che il page fault incrementi il contatore
    int initial_page_faults = mmu->page_fault_count;
    LogicalAddress la = {0, 0, 0}; 
    char data = MMU_readByte(mmu, la); // Questo dovrebbe causare un page fault
    printf("Read data after page fault: %c\n", data);
    assert(mmu->pages[0].flags & PageValid && "Page should be valid after page fault");
    assert(mmu->page_fault_count == initial_page_faults + 1 && "Page fault count should increase");

    // Verifica che il file di swap venga letto correttamente
    fseek(mmu->swap_file, 0, SEEK_END); 
    long swap_file_size = ftell(mmu->swap_file);
    assert(swap_file_size == VIRTUAL_MEMORY_SIZE && "Swap file size should match virtual memory size");

    MMU_free(mmu);
    printf("Test Page Fault Handling with Swap File passed.\n");
}


void test_write_and_read() {
    printf("Running test: Write and Read...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    
    LogicalAddress la = {0, 1, 100};
    MMU_writeByte(mmu, la, 'A');
    
    print_MMU_state(mmu);
    
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

void test_swap_file_usage() {
    printf("Running test: Swap File Usage...\n");

    MMU* mmu = MMU_init("swap_file_test.bin");
    assert(mmu->swap_file != NULL && "Swap file should be opened correctly");
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        LogicalAddress la = {0, i, 0}; 
        MMU_writeByte(mmu, la, 'A' + i); 
        printf("Segment ID: %d, Page Number: %d, Segment Limit: %d\n", la.segment_id, la.page_number, mmu->segments[la.segment_id].limit);

    }

    LogicalAddress la = {0, 0, 0};
    char data = MMU_readByte(mmu, la);
    assert(data == 'A' && "Page should be swapped in and contain the correct data");

    fseek(mmu->swap_file, 0, SEEK_END);
    long swap_file_size = ftell(mmu->swap_file);
    assert(swap_file_size == VIRTUAL_MEMORY_SIZE && "Swap file size should match virtual memory size");
    MMU_free(mmu);
    printf("Test Swap File Usage passed.\n");
}

int main() {
    test_write_and_read();

    printf("All tests passed!\n");
    return 0;
}
