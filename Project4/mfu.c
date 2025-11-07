#include <stdio.h>
#include <stdlib.h>
#include "common.h"


int MFU(Memory *memory, char processName, int pageNumber, int timestamp)
{
  for (int i = 0; i < memory->count; i++)
  {
    if (memory->pages[i].processName == processName && memory->pages[i].pageNumber == pageNumber)
    {
      (memory->pages[i].frequency)++;
      memory->pages[i].lastUsed = timestamp;
      return 0;
    }
  }

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
    int mfuIndex = 0;
    int mostFreq = memory->pages[0].frequency;

    for (int i = 1; i < memory->count; i++)
    {
      if (memory->pages[i].frequency > mostFreq)
      {
        mostFreq = memory->pages[i].frequency;
        mfuIndex = i;
      }
    }
    memory->pages[mfuIndex].processName = processName;
    memory->pages[mfuIndex].pageNumber = pageNumber;
    memory->pages[mfuIndex].lastUsed = timestamp;
    memory->pages[mfuIndex].frequency = 1;
    memory->pages[mfuIndex].timestamp = timestamp;
  }

  return 1;
}

