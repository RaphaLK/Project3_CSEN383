#include <stdio.h>
#include <stdlib.h>

int main() {
    JobQueue jobQueue;
    generateWorkload(&jobQueue); 

    int (*algorithms[])(Memory *, char, int, int) = {FIFO, LRU, LFU, MFU, RandomPick};
    char *algorithmNames[] = {"FIFO", "LRU", "LFU", "MFU", "RandomPick"};

    for (int i = 0; i < 5; i++) {
        double hitRatioSum = 0, missRatioSum = 0;
        int swappedInSum = 0;

        for (int run = 0; run < 5; run++) {
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