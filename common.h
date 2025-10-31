#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PAGES 100            
#define MAX_JOBS 150             
#define PAGE_SIZE 1              // Each page = 1 MB
#define SIMULATION_TIME 60       
#define REFERENCE_INTERVAL 100   // Page reference every 100 ms

typedef struct Job {
    char processName;            
    int processSize;             
    int arrivalTime;             
    int serviceDuration;         
    int currentPage;             
    struct Job* next;            // Pointer to next job
} Job;

// Page structure used in memory
typedef struct {
    char processName;            // Process that owns the page
    int pageNumber;              
    int lastUsed;                // Used for LRU
    int frequency;               // Used for LFU and MFU
    int timestamp;               // Used for FIFO
} Page;

// Memory structure
typedef struct {
    Page pages[MAX_PAGES];       // Array of pages in memory
    int count;                   // Number of used pages
} Memory;

// Job queue
typedef struct {
    Job* head;                   
    int count;                  
} JobQueue;

// Function declarations
void initializeMemory(Memory *memory);
void generateWorkload(JobQueue *jobQueue);
void runSimulation(JobQueue *jobQueue, Memory *memory, int (*replacementAlgorithm)(Memory *, char, int, int), char *algorithmName, double *hitRatioSum, double *missRatioSum, int *swappedInSum);
int FIFO(Memory *memory, char processName, int pageNumber, int timestamp);
int LRU(Memory *memory, char processName, int pageNumber, int timestamp);
int LFU(Memory *memory, char processName, int pageNumber, int timestamp);
int MFU(Memory *memory, char processName, int pageNumber, int timestamp);
int RandomPick(Memory *memory, char processName, int pageNumber, int timestamp);
int generateNextPageReference(int currentPage, int processSize);
void removeJobPages(Memory *memory, char processName);
void runSimulation(JobQueue *jobQueue, Memory *memory, int (*replacementAlgorithm)(Memory *, char, int, int), char *algorithmName, double *hitRatioSum, double *missRatioSum, int *swappedInSum);

#endif // COMMON_H