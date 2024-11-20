CC=gcc
CCOPTS=--std=gnu99 -g -Wall 
AR=ar

BINS=fake_mmu_test

OBJS=fake_mmu.o

HEADERS=fake_mmu.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all valgrind

all:	$(BINS)

fake_mmu_test: fake_mmu_test.c fake_mmu.c 
	$(CC) $(CCOPTS) -D_FAKE_MMU_TEST_ -o $@ $^

valgrind: $(BINS)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BINS)

clean:
	rm -rf *.o *~  $(BINS)
	rm -rf swap_file.bin 
	rm -rf schema.csv
