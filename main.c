#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int main()
{
    JobQueue jobQueue;
    generateWorkload(&jobQueue);

    // int (*algorithms[])(Memory *, char, int, int) = {FIFO, LRU, LFU, MFU, RandomPick};
    // int (*algorithms[])(Memory *, char, int, int) = {FIFO, LRU, LFU, RandomPick};
    int (*algorithms[])(Memory *, char, int, int) = {FIFO};

    // char *algorithmNames[] = {"FIFO", "LRU", "LFU", "MFU", "RandomPick"};
    // char *algorithmNames[] = {"LRU", "LFU", "RandomPick"};
    char *algorithmNames[] = {"FIFO"};

    // Change the constant as we implement the other algos pls.
    int numSims = 1;
    for (int i = 0; i < numSims; i++)
    {
        double hitRatioSum = 0, missRatioSum = 0;
        int swappedInSum = 0;

        for (int run = 0; run < 5; run++)
        {
            Memory memory; // Start with a clean memory state for this run
            initializeMemory(&memory);
            runSimulation(&jobQueue, &memory, algorithms[i], algorithmNames[i], &hitRatioSum, &missRatioSum, &swappedInSum);
        }

        double avgHitRatio = hitRatioSum / 5;
        double avgMissRatio = missRatioSum / 5;
        double avgSwappedIn = (double)swappedInSum / 5;

        printf("\nAverage Results for %s Algorithm \n", algorithmNames[i]);
        printf("Average Hit Ratio: %.2f\n", avgHitRatio);
        printf("Average Miss Ratio: %.2f\n", avgMissRatio);
        printf("Average Processes Swapped-In: %.2f\n", avgSwappedIn);
    }

    return 0;
}

void initializeMemory(Memory *memory)
{
    memory->count = 0;
    // Initialize all pages to empty state
    for (int i = 0; i < MAX_PAGES; i++)
    {
        memory->pages[i].processName = '\0';
        memory->pages[i].pageNumber = -1;
        memory->pages[i].lastUsed = -1;
        memory->pages[i].frequency = 0;
        memory->pages[i].timestamp = -1;
    }
}

void generateWorkload(JobQueue *jobQueue)
{
    srand(time(NULL));

    jobQueue->head = NULL;
    jobQueue->count = 0;

    // Process sizes: 5, 11, 17, 31 MB (pages)
    int processSizes[] = {5, 11, 17, 31};

    // Service durations: 1, 2, 3, 4, 5 seconds
    int serviceDurations[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < MAX_JOBS; i++)
    {
        Job *newJob = (Job *)malloc(sizeof(Job));
        if (!newJob)
        {
            printf("Memory allocation failed for job %d\n", i);
            continue;
        }

        // Process name: A-Z, then AA-ZZ, etc.
        if (i < 26)
        {
            newJob->processName = 'A' + i;
        }
        else
        {
            newJob->processName = 'A' + (i % 26);
        }

        newJob->processSize = processSizes[rand() % 4];
        newJob->arrivalTime = rand() % SIMULATION_TIME;
        newJob->serviceDuration = serviceDurations[rand() % 5];
        newJob->currentPage = 0;
        newJob->next = NULL;

        // sorted insert
        if (jobQueue->head == NULL || newJob->arrivalTime < jobQueue->head->arrivalTime)
        {
            newJob->next = jobQueue->head;
            jobQueue->head = newJob;
        }
        else
        {
            Job *current = jobQueue->head;
            while (current->next != NULL && current->next->arrivalTime <= newJob->arrivalTime)
            {
                current = current->next;
            }
            newJob->next = current->next;
            current->next = newJob;
        }
        jobQueue->count++;
    }
}

void generateMemoryMap(Memory *memory, char *mapString)
{
    // Initialize all positions to '.' (holes)
    for (int i = 0; i < MAX_PAGES; i++)
    {
        mapString[i] = '.';
    }

    // Fill with process names
    for (int i = 0; i < memory->count; i++)
    {
        if (memory->pages[i].processName != '\0')
        {
            mapString[i] = memory->pages[i].processName;
        }
    }
    mapString[MAX_PAGES] = '\0'; // Null terminate
}

void runSimulation(JobQueue *jobQueue, Memory *memory,
                   int (*replacementAlgorithm)(Memory *, char, int, int),
                   char *algorithmName, double *hitRatioSum, double *missRatioSum,
                   int *swappedInSum)
{

    int totalReferences = 0;
    int totalHits = 0;
    int totalMisses = 0;
    int swappedInProcesses = 0;
    int currentTime = 0;
    char memoryMap[MAX_PAGES + 1];

    // total mem = 100, each process needs 4 free pages to start, 25 max processes running simulatenously
    Job *runningJobs[25]; // Maximum 25 jobs can run simultaneously
    int runningJobsCount = 0;
    int jobEndTimes[25];

    Job *currentJob = jobQueue->head;

    printf("\nRunning %s Algorithm Simulation\n", algorithmName);
    printf("Time\tProcess\tPage\tResult\tEvicted\n");
    printf("----\t-------\t----\t------\t-------\n");

    // Main simulation loop
    for (currentTime = 0; currentTime < SIMULATION_TIME * 1000; currentTime += REFERENCE_INTERVAL)
    {

        // Check for new job arrivals (need at least 4 free pages)
        while (currentJob && currentJob->arrivalTime * 1000 <= currentTime &&
               runningJobsCount < 25 && (MAX_PAGES - memory->count) >= 4)
        {
            generateMemoryMap(memory, memoryMap);
            printf("%.1f\t%c\tEnter\t%d\t%d\t<%s>\n",
                   currentTime / 1000.0,
                   currentJob->processName,
                   currentJob->processSize,
                   currentJob->serviceDuration,
                   memoryMap);
            runningJobs[runningJobsCount] = currentJob;
            jobEndTimes[runningJobsCount] = currentTime + (currentJob->serviceDuration * 1000);
            runningJobsCount++;
            swappedInProcesses++;

            // Load initial page (page 0)
            int result = replacementAlgorithm(memory, currentJob->processName, 0, currentTime);
            totalReferences++;
            if (result == 0)
                totalHits++;
            else
                totalMisses++;

            printf("\n%.1f\t%c\t%d\t%s\t-\n",
                   currentTime / 1000.0, currentJob->processName, 0,
                   result ? "MISS" : "HIT");

            currentJob = currentJob->next;
        }

        // Process memory references for running jobs
        for (int i = 0; i < runningJobsCount; i++)
        {
            if (currentTime < jobEndTimes[i])
            {
                Job *job = runningJobs[i];

                // Generate next page reference using locality of reference
                int nextPage = generateNextPageReference(job->currentPage, job->processSize);
                job->currentPage = nextPage;

                // Make memory reference
                int result = replacementAlgorithm(memory, job->processName, nextPage, currentTime);
                totalReferences++;
                if (result == 0)
                    totalHits++;
                else
                    totalMisses++;

                printf("%.1f\t%c\t%d\t%s\t-\n",
                       currentTime / 1000.0, job->processName, nextPage,
                       result ? "MISS" : "HIT");
            }
        }

        // Remove completed jobs
        for (int i = 0; i < runningJobsCount; i++)
        {
            if (currentTime >= jobEndTimes[i])
            {
                generateMemoryMap(memory, memoryMap);
                printf("\n%.1f\t%c\tExit\t%d\t%d\t<%s>\n",
                       currentTime / 1000.0,
                       runningJobs[i]->processName,
                       runningJobs[i]->processSize,
                       runningJobs[i]->serviceDuration,
                       memoryMap);

                // Remove job's pages from memory
                removeJobPages(memory, runningJobs[i]->processName);

                // Shift remaining jobs
                for (int j = i; j < runningJobsCount - 1; j++)
                {
                    runningJobs[j] = runningJobs[j + 1];
                    jobEndTimes[j] = jobEndTimes[j + 1];
                }
                runningJobsCount--;
                i--;
            }
        }
    }

    // Calculate statistics
    double hitRatio = totalReferences > 0 ? (double)totalHits / totalReferences : 0.0;
    double missRatio = totalReferences > 0 ? (double)totalMisses / totalReferences : 0.0;

    *hitRatioSum += hitRatio;
    *missRatioSum += missRatio;
    *swappedInSum += swappedInProcesses;

    printf("\nSimulation Results:\n");
    printf("Total References: %d\n", totalReferences);
    printf("Hits: %d, Misses: %d\n", totalHits, totalMisses);
    printf("Hit Ratio: %.2f, Miss Ratio: %.2f\n", hitRatio, missRatio);
    printf("Processes Swapped In: %d\n", swappedInProcesses);
}

// Generate next page reference with 70% locality
int generateNextPageReference(int currentPage, int processSize)
{
    int r = rand() % 11; // 0-10
    int nextPage;

    if (r < 7)
    {                                 // 70% probability: locality of reference
        int delta = (rand() % 3) - 1; // -1, 0, or 1
        nextPage = currentPage + delta;

        // Handle wraparound
        if (nextPage < 0)
            nextPage = processSize - 1;
        if (nextPage >= processSize)
            nextPage = 0;
    }
    else
    { // 30% probability: random jump
        do
        {
            nextPage = rand() % processSize;
        } while (abs(nextPage - currentPage) < 2); // Ensure |Δi| >= 2
    }

    return nextPage;
}

// Remove all pages of a completed job from memory
void removeJobPages(Memory *memory, char processName)
{
    for (int i = 0; i < memory->count; i++)
    {
        if (memory->pages[i].processName == processName)
        {
            // Shift remaining pages down
            for (int j = i; j < memory->count - 1; j++)
            {
                memory->pages[j] = memory->pages[j + 1];
            }
            memory->count--;
            i--; // Recheck this position
        }
    }
}
