#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int FIFO(Memory *memory, char processName, int pageNumber, int timestamp)
{
  // page hit
  for (int i = 0; i < memory->count; i++)
  {
    if (memory->pages[i].processName == processName &&
        memory->pages[i].pageNumber == pageNumber)
    {
      return 0;
    }
  }

  // page miss
  if (memory->count < MAX_PAGES)
  {
    // add page to memory
    memory->pages[memory->count].processName = processName;
    memory->pages[memory->count].pageNumber = pageNumber;
    memory->pages[memory->count].timestamp = timestamp;
    memory->pages[memory->count].frequency = 1;
    memory->pages[memory->count].lastUsed = timestamp;
    memory->count++;
    return 1;
  }
  else
  {
    // replacement, oldest page first
    int oldestIndex = 0;
    for (int i = 1; i < memory->count; i++)
    {
      if (memory->pages[i].timestamp < memory->pages[oldestIndex].timestamp)
      {
        oldestIndex = i;
      }
    }
    memory->pages[oldestIndex].processName = processName;
    memory->pages[oldestIndex].pageNumber = pageNumber;
    memory->pages[oldestIndex].timestamp = timestamp;
    memory->pages[oldestIndex].frequency = 1;
    memory->pages[oldestIndex].lastUsed = timestamp;
    return oldestIndex;
  }
}
