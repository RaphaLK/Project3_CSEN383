/******************************************************************
 * Simulate ticket sellers simultaneously selling concert tickets for 1 hour
 * 3 types of tickets
 * - H tickets (High-priced) -> 1 seller -> Takes 1 or 2 minutes to complete sale (random)
 * - M tickets (Medium Priced) -> 3 sellers -> Takes 2, 3, 4 minutes to complete sale (random)
 * - L tickets (Low priced) -> 6 sellers -> Takes 4, 5, 6, 7 minutes to complete sale (random)
 * 
 * High Seller -> Starts from row 1 -> row 10
 * Medium Seller -> 5, 6, 4, 7
 * Low Seller -> Row 10 -> Row 1
 * Parameter: N --> Number of customers expected to arrive at random times during the hour
 * Customers can only arrive at the start of a minute
 * 
 * Output:
 * - When a customer arrives at the tail of a seller's queue
 * - A customer is served and is assigned a seat, or is told the concert is sold out
 *  - The customer will immediately leave if the concert is sold out
 * - A customer completes a ticket pruchase and leaves
 * - Program should compute the avg response time per customer per given seller type, 
 * average turn-around time per customer for a given seller type, average throughput per seller type.
 * 
 ********************************************************************/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define HOUR 60 
#define NUM_SEATS 100

int time = 0;
typedef struct statistics {
  float averageResponseTime;
  float averageTurnaroundTime;
  float averageThroughput;
} statistics;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Please supply a second commmand line param\n\n");
    return -1;
  }


}