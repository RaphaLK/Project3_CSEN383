#include <stdio.h>
#include <stdlib.h>
#include "common.h"


int LFU(Memory *memory, char processName, int pageNumber, int timestamp)
{
  for (int i = 0; i < memory->count; i++)
  {
    // Checks if a page is already in memory, in that case just update the frequency
    if (memory->pages[i].processName == processName && memory->pages[i].pageNumber == pageNumber)
    {
      (memory->pages[i].frequency)++;
      return 0;
    }
  }

  // page fault -- memory available, and memory not available, so kick out the lowest frequency
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
    int lfuIndex = 0;
    int leastFreq = memory->pages[0].frequency;

    for (int i = 1; i < memory->count; i++)
    {
      if (memory->pages[i].frequency < leastFreq)
      {
        leastFreq = memory->pages[i].frequency;
        lfuIndex = i;
      }
    }
    memory->pages[lfuIndex].processName = processName;
    memory->pages[lfuIndex].pageNumber = pageNumber;
    memory->pages[lfuIndex].lastUsed = timestamp;
    memory->pages[lfuIndex].frequency = 1;
    memory->pages[lfuIndex].timestamp = timestamp;
  }

  return 1;
}
