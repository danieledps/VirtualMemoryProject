#include "fake_mmu.h"
#include <stdio.h>
#include <assert.h>

void test_mmu_with_swap_handling() {

    MMU* mmu = MMU_init("swapfile_test.bin");

    LogicalAddress addr1 = {0, 0, 0}; 
    LogicalAddress addr2 = {0, 1, 0};  
    LogicalAddress addr3 = {0, 2, 0};  
    LogicalAddress addr4 = {0, 3, 0}; 
    LogicalAddress addr5 = {0, 4, 0}; 

    mmu->segments[0].base = 0;
    mmu->segments[0].limit = 5;
    mmu->segments[0].flags = Valid | Read | Write;

    printf("\n=== TEST WRITE WITH PAGE FAULT ===\n");
    MMU_writeByte(mmu, addr1, 'A'); 
    MMU_writeByte(mmu, addr2, 'B'); 
    MMU_writeByte(mmu, addr3, 'C'); 
    MMU_writeByte(mmu, addr4, 'D'); 
    printf("\n=== CAUSING PAGE FAULT (OUT OF MEMORY) ===\n");
    MMU_writeByte(mmu, addr5, 'E'); 

    printf("\n=== TEST READ AFTER SWAP ===\n");
    char c1 = MMU_readByte(mmu, addr1);
    char c2 = MMU_readByte(mmu, addr2);
    char c3 = MMU_readByte(mmu, addr3);
    char c4 = MMU_readByte(mmu, addr4);
    char c5 = MMU_readByte(mmu, addr5); 

    printf("Expected 'A', got: '%c'\n", c1);
    printf("Expected 'B', got: '%c'\n", c2);
    printf("Expected 'C', got: '%c'\n", c3);
    printf("Expected 'D', got: '%c'\n", c4);
    printf("Expected 'E', got: '%c'\n", c5);

    MMU_free(mmu);
}

int main() {
    test_mmu_with_swap_handling();
    return 0;
}
