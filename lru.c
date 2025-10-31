#include <stdio.h>
#include <stdlib.h>
#include "common.h"


int LRU(Memory *memory, char processName, int pageNumber, int timestamp)
{
  for (int i = 0; i < memory->count; i++)
  {
    // Checks if a page is already in memory, in that case just update the last retrieved time
    if (memory->pages[i].processName == processName && memory->pages[i].pageNumber == pageNumber)
    {
      memory->pages[i].lastUsed = timestamp;
      return 0;
    }
  }

  // page fault -- memory available, and memory not available, so kick out the oldest page
  if (memory->count < MAX_PAGES)
  {
    memory->pages[memory->count].processName = processName;
    memory->pages[memory->count].pageNumber = pageNumber;
    memory->pages[memory->count].lastUsed = timestamp;
    memory->pages[memory->count].frequency = 1;
    memory->pages[memory->count].timestamp = timestamp;;
    memory->count++;
  }
  else
  {
    int lruIndex = 0;
    int oldestTime = memory->pages[0].lastUsed;

    for (int i = 1; i < memory->count; i++)
    {
      if (memory->pages[i].lastUsed < oldestTime)
      {
        oldestTime = memory->pages[i].lastUsed;
        lruIndex = i;
      }
    }
    memory->pages[lruIndex].processName = processName;
    memory->pages[lruIndex].pageNumber = pageNumber;
    memory->pages[lruIndex].lastUsed = timestamp;
    memory->pages[lruIndex].frequency = 1;
    memory->pages[lruIndex].timestamp = timestamp;
  }

  return 1;
}