#include "fake_mmu.h"
#include <stdio.h>
#include <assert.h>

#define NUM_TEST_PAGES 4

int main() {
    MMU* mmu = MMU_init("swapfile");

    for (int i = 0; i < NUM_TEST_PAGES; i++) {
        mmu->segments[i].base = 0;
        mmu->segments[i].limit = NUM_TEST_PAGES;
        mmu->segments[i].flags = Valid | Read | Write;

        mmu->pages[i].frame_number = i; 
        mmu->pages[i].flags = PageValid;
    }

    LogicalAddress logical_address;
    char data[NUM_TEST_PAGES] = {'A', 'B', 'C', 'D'};

    for (int i = 0; i < NUM_TEST_PAGES; i++) {
        logical_address.segment_id = 0;
        logical_address.page_number = i;
        logical_address.offset = 0;

        MMU_writeByte(mmu, logical_address, data[i]);
        char read_value = MMU_readByte(mmu, logical_address);
        assert(read_value == data[i] && "Errore nella lettura!");
    }

    // Test eccezione
    logical_address.page_number = 2; 
    mmu->pages[logical_address.page_number].flags = 0; // Imposta la pagina come non valida

    printf("Tentativo di scrittura in pagina non valida %d...\n", logical_address.page_number);
    MMU_exception(mmu, logical_address.page_number); 
    printf("Page fault gestito per la pagina %d.\n", logical_address.page_number);

    mmu->pages[logical_address.page_number].flags = PageValid; // Imposta la pagina a valida
    MMU_writeByte(mmu, logical_address, 'X');
    char read_value_after_fault = MMU_readByte(mmu, logical_address);
    assert(read_value_after_fault == 'X' && "Errore nella lettura dopo il page fault!");

    MMU_free(mmu);
    return 0;
}
