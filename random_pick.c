#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int RandomPick(Memory *memory, char processName, int pageNumber, int timestamp) 
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

  // page fault --  kick out a random page
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
    int randomIndex = rand() % memory->count;

    memory->pages[randomIndex].processName = processName;
    memory->pages[randomIndex].pageNumber = pageNumber;
    memory->pages[randomIndex].lastUsed = timestamp;
    memory->pages[randomIndex].frequency = 1;
    memory->pages[randomIndex].timestamp = timestamp;
  }

  return 1;
}