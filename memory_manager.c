#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "memory_manager.h"

GlobalMemoryLayout m;

/**
   initializes the global memory layout
   the free pages list contains all memory
   the process list contains no processes
*/
void Memory_init()
{

  //1. fill the pages with pid -1 (invalid)
  //2. fill the list of free chunks to cover all memory

  for (int i = 0; i < PAGES_NUM; ++i)
  {
    m.frame_to_pid[i] = -1;
    FrameItem *pitem = FrameItem_alloc();
    FrameItem_init(pitem, -1, i);
    List_insert(&m.frame_list, m.frame_list.last, (ListItem *)pitem);
  }
  //3. clear the process list
  List_init(&m.process_list);
  printf("free frames: %d\n", m.frame_list.size);
}

void Memory_shutdown()
{
  // we release all process resources
  while (m.process_list.first)
  {
    ProcessMemoryLayout *process = (ProcessMemoryLayout *)m.process_list.first;
    Memory_destroyProcessMemoryLayout(process);
  }
  // we release the memory list
  while (m.frame_list.first)
  {
    FrameItem *frame = (FrameItem *)m.frame_list.first;
    List_detach(&m.frame_list, m.frame_list.first);
    FrameItem_free(frame);
  }
}

/**
   retrieves if existing the process layout of pid
*/
ProcessMemoryLayout *Memory_byPid(int pid)
{
  return ProcessMemoryList_byPid(&m.process_list, pid);
}

/**
   adds a process with pid to the list. The process has no memory allocated and no segments
 */
ProcessMemoryLayout *Memory_addProcessLayout(int pid)
{
  //1. we seek if another process with the same pid exists
  assert(!Memory_byPid(pid) && "invalid_pid");
  ProcessMemoryLayout *new_process = ProcessMemoryLayout_alloc();
  assert(new_process && "cant allocate process memory layout");
  ProcessMemoryLayout_init(new_process, pid);
  List_insert(&m.process_list, m.process_list.last, (ListItem *)new_process);
  return new_process;
}

/**
   returns, if present a segment having segment num in the list
*/
SegmentItem *Memory_getSegment(ProcessMemoryLayout *pmem, int segment_id)
{
  return SegmentList_byId(&pmem->segments_list, segment_id);
}

/**
   adds if possible a memory segment to the process, whose segment id is segment_num.
   the segment has base and limit set to 0 (no size)
*/
SegmentItem *Memory_addSegment(ProcessMemoryLayout *pmem,
                               uint32_t segment_id,
                               uint32_t base,
                               uint32_t limit)
{
  //1. if the segment exists in the process we return an error
  // we lookup in the segment map of the process, and see if a segment with a valid
  // bit exists
  SegmentDescriptor *des = pmem->segments + segment_id;

  // if a segment descriptor not marked as invalid is in the pool,
  // the process already has a segment with that id, so we return a failure
  if ((des->flags & Valid))
  {
    printf("descriptor in use\n");
    return 0;
  }
  // we need to check that there is enough free physical memory
  if (m.frame_list.size < limit)
  {
    printf("no memory use\n");
    return 0;
  }
  // we need to check that there is enough contiguous space in the
  // process address space between base and limit
  for (uint32_t p = base; p < base + limit; ++p)
  {
    if (pmem->pages[p].flags & Valid)
    {
      printf("no contiguous memory\n");
      return 0;
    }
  }

  SegmentItem *segment = SegmentItem_alloc();
  SegmentItem_init(segment, pmem, segment_id);

  // we start moving memory from the free zone
  for (uint32_t p = base; p < base + limit; ++p)
  {
    ListItem *aux = m.frame_list.first;

    // the list is empty before we expect it to be so
    assert(aux && "bookeeping error: empty free list");

    FrameItem *new_frame = (FrameItem *)aux;
    uint32_t frame_num = new_frame->frame_num;

    // the frame is not assigned to other processes
    assert(m.frame_to_pid[frame_num] < 0 && "bookeeping error: frame already assigned");
    new_frame->pid = pmem->pid;
    // assign the pid to the
    m.frame_to_pid[frame_num] = pmem->pid;

    // we move the memory in the process region
    List_detach(&m.frame_list, m.frame_list.first);
    List_insert(&segment->frame_list, segment->frame_list.last, (ListItem *)new_frame);

    pmem->pages[p].frame_number = frame_num;
    pmem->pages[p].flags |= Valid;
  }

  // all fine, we can ask for memory pages and map them to the segment
  List_insert(&pmem->segments_list, pmem->segments_list.last, (ListItem *)segment);

  // we alter the segment table
  pmem->segments[segment_id].base = base;
  pmem->segments[segment_id].limit = limit;
  pmem->segments[segment_id].flags |= Valid;
  return segment;
}

/**
   removes a segment to a process and frees all pages occupied by a segment
   it updates the pid in the memory list, to reflect that the segment has been augmented
*/
int Memory_destroySegment(SegmentItem *segment)
{
  ProcessMemoryLayout *pmem = segment->pmem;
  SegmentDescriptor des = pmem->segments[segment->segment_id];
  assert((des.flags & Valid) && "releasing invalid segment");
  uint32_t base = des.base;
  uint32_t limit = des.limit;
  int pid = segment->pmem->pid;

  //  we move all pages in the segment in free list
  while (segment->frame_list.first)
  {
    ListItem *aux = segment->frame_list.first;
    List_detach(&segment->frame_list, segment->frame_list.first);
    List_insert(&m.frame_list, m.frame_list.last, aux);
    FrameItem *frame_item = (FrameItem *)aux;
    uint32_t frame_num = frame_item->frame_num;
    assert(m.frame_to_pid[frame_num] == pid && "bookeeping, releasing memory from wrong process");
    m.frame_to_pid[frame_num] = -1;
    frame_item->pid = -1;
    limit--;
  }
  printf("%d", limit);
  assert(!limit && "bookeeping, wrong number of pages released");

  // we walk through the process memory table and we free the pages
  for (int p = base; p < base + des.limit; ++p)
  {
    pmem->pages[p].flags &= (~Valid);
  }
  List_detach(&pmem->segments_list, (ListItem *)segment);
  SegmentItem_free(segment);
  return 0;
}

uint32_t Memory_freePages() { return m.frame_list.size; }

void Memory_destroyProcessMemoryLayout(ProcessMemoryLayout *pmem)
{
  while (pmem->segments_list.size)
  {
    ListItem *aux = pmem->segments_list.first;
    SegmentItem *segment = (SegmentItem *)aux;
    aux = aux->next;
    Memory_destroySegment(segment);
  }
  List_detach(&m.process_list, (ListItem *)pmem);
  ProcessMemoryLayout_free(pmem);
}

SegmentItem *Memory_moveSegment(SegmentItem *segment, uint32_t new_base)
{
  return 0;
}

SegmentItem *Memory_resizeSegment(SegmentItem *segment, uint32_t new_limit)
{
  return 0;
}
