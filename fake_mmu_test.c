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
    MMU_exportToCSV(mmu, "schema.csv");
    MMU_free(mmu);
    printf("Test MMU_initialization passed.\n");
}

void test_page_fault_handling_with_swap() {
    printf("Running test: Page Fault Handling with Swap File...\n");
    MMU* mmu = MMU_init("swap_file_test.bin");
    assert(mmu->swap_file != NULL && "Swap file should be opened correctly");
    
    int initial_page_faults = mmu->page_fault_count;
    LogicalAddress la = {0, 0, 0}; 
    char data = MMU_readByte(mmu, la); 
    printf("Read data after page fault: %c\n", data);
    assert(mmu->pages[0].flags & PageValid && "Page should be valid after page fault");
    assert(mmu->page_fault_count == initial_page_faults + 1 && "Page fault count should increase");

    fseek(mmu->swap_file, 0, SEEK_END); 
    long swap_file_size = ftell(mmu->swap_file);
    assert(swap_file_size == VIRTUAL_MEMORY_SIZE && "Swap file size should match virtual memory size");
    MMU_exportToCSV(mmu, "schema.csv");
    MMU_free(mmu);
    printf("Test Page Fault Handling with Swap File passed.\n");
}

void test_write_and_read() {
    printf("Running test: Write and Read...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    
    LogicalAddress la = {0, 1, 100};
    MMU_writeByte(mmu, la, 'A');
    
    char c = MMU_readByte(mmu, la);
    assert(c == 'A' && "The byte read should match the written byte");
    MMU_exportToCSV(mmu, "schema.csv");
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
    MMU_exportToCSV(mmu, "schema.csv");
    MMU_free(mmu);
    printf("Test Second Chance Algorithm passed.\n");
}

void test_access_out_of_segment() {
    printf("Running test: Access Out Of Segment...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    LogicalAddress la = {0, 257, 0};
    printf("Segment ID: %d, Page Number: %d, Segment Limit: %d\n", la.segment_id, la.page_number, mmu->segments[la.segment_id].limit);
    assert(!(la.page_number < mmu->segments[la.segment_id].limit) && "Access should fail due to out of bounds");
    MMU_exportToCSV(mmu, "schema.csv");
    MMU_free(mmu);
    printf("Test Access Out Of Segment passed.\n");
}

void test_swap_file_usage() {
    printf("Running test: Swap File Usage...\n");
    MMU* mmu = MMU_init("swap_file.bin");
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
    MMU_exportToCSV(mmu, "schema.csv");
    MMU_free(mmu);
    printf("Test Swap File Usage passed.\n");
}

void test_multiple_segments() {
    printf("Running test: Multiple Segments Usage...\n");
    MMU* mmu = MMU_init("swap_file.bin");
    assert(mmu->swap_file != NULL && "Swap file should be opened correctly");

    int pages_to_write = 400;
    char base_char = 'A';  
    for (int i = 0; i < pages_to_write; i++) {
        int segment_id = i / (NUM_PAGES / NUM_SEGMENTS);
        int page_number = i % (NUM_PAGES / NUM_SEGMENTS);
        LogicalAddress la = {segment_id, page_number, 0}; 
        MMU_writeByte(mmu, la, base_char + (i % 26));  
        printf("Writing to Segment %d, Page %d, Char: %c\n", segment_id, page_number, base_char + (i % 26));
    }

    MMU_exportToCSV(mmu, "schema.csv");
    for (int i = 0; i < pages_to_write; i++) {
        int segment_id = i / (NUM_PAGES / NUM_SEGMENTS);
        int page_number = i % (NUM_PAGES / NUM_SEGMENTS);
        LogicalAddress la = {segment_id, page_number, 0}; 
        char read_data = MMU_readByte(mmu, la);
        char expected_data = base_char + (i % 26);  
        printf("Expected: %c, Read: %c from Segment %d, Page %d\n", expected_data, read_data, segment_id, page_number);
        
        assert(read_data == expected_data && "Page should contain the correct data");
    }

    MMU_free(mmu);
}

void test_page_valid_flag() {
    MMU* mmu = MMU_init("swap_file.bin");

    LogicalAddress addr = { .segment_id = 0, .page_number = 0, .offset = 0 };
    
    MMU_writeByte(mmu, addr, 'A');
    assert(mmu->pages[addr.page_number].flags & PageValid);
    printf("PageValid impostato correttamente dopo il page fault.\n");

    LogicalAddress addr2 = { .segment_id = 0, .page_number = 1, .offset = 0 };
    MMU_writeByte(mmu, addr2, 'B');

    assert(!(mmu->pages[addr.page_number].flags & PageValid) && "Errore: PageValid non è stato disattivato.");
    printf("PageValid rimosso correttamente dopo l'espulsione della pagina.\n");

    char value = MMU_readByte(mmu, addr);
    assert((mmu->pages[addr.page_number].flags & PageValid) && "Errore: PageValid non è stato impostato correttamente dopo il secondo page fault.");
    printf("Rilettura: PageValid impostato correttamente dopo il secondo page fault.\n");
    assert(value == 'A' && "Errore: il valore letto non corrisponde al valore scritto inizialmente.");

    MMU_free(mmu);
}


void interactive_menu() {
    int choice;

    do {
        printf("=== MMU Test Menu ===\n");
        printf("1. Test MMU Initialization\n");
        printf("2. Test Page Fault Handling\n");
        printf("3. Test Write and Read\n");
        printf("4. Test Second Chance Algorithm\n");
        printf("5. Test Access Out Of Segment\n");
        printf("6. Test Swap File Usage\n");
        printf("7. Test Multiple Segments\n");
        printf("8. Exit\n");
        printf("Select a test to run (1-8): ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                test_MMU_initialization();
                break;
            case 2:
                test_page_fault_handling_with_swap();
                break;
            case 3:
                test_write_and_read();
                break;
            case 4:
                test_second_chance_algorithm();
                break;
            case 5:
                test_access_out_of_segment();
                break;
            case 6:
                test_swap_file_usage();
                break;
            case 7:
                test_multiple_segments();
                break;
            case 8:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice. Please select a number between 1 and 7.\n");
        }
    } while (choice != 8);
}

int main() {
    test_page_valid_flag();
    interactive_menu();
    return 0;
}
