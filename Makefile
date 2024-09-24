CC=gcc
CCOPTS=--std=gnu99 -g -Wall 
AR=ar


BINS=fake_mmu_test

OBJS=linked_list.o fake_mmu.o

HEADERS=linked_list.h fake_mmu.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

fake_mmu_test: fake_mmu_test.c fake_mmu.c 
	$(CC) $(CCOPTS) -D_FAKE_MMU_TEST_ -o $@ $^

clean:
	rm -rf *.o *~  $(BINS)
