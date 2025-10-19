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

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int done = 0;
pthread_cond_t all_done = PTHREAD_COND_INITIALIZER;

int minute = 0;

typedef struct customer
{
  int id;
  customer *next;
} customer;

// Queue using a LL implementation
typedef struct queue
{
  customer *front;
  customer *rear;
  int size;
  char *s_id;
  pthread_mutex_t q_lock;
} queue;

typedef struct statistics
{
  float averageResponseTime;
  float averageTurnaroundTime;
  float averageThroughput;
} statistics;

// We need the aggregate response time, turnaround, and throughput for each type of seller later.
typedef struct seller
{
  char *s_id;
  statistics stats;
} seller;

queue *h_price;
queue *m_price[3];
queue *l_queues[6];
seller *sellers[10];

void *sell(void *s_t)
{
  pthread_mutex_lock(&mutex);
  done++;
  if (done == 10)
  {
    pthread_cond_signal(&all_done);
  }
  pthread_cond_wait(&cond, &mutex);

  pthread_mutex_unlock(&mutex);
  printf("%s\n", (char *)s_t);

  return NULL;
}

void setup_queue(char *seller_name)
{
  queue *q = malloc(sizeof(queue));
  q->s_id = seller_name;
  q->front = NULL;
  q->rear = NULL;
  q->size = 0;
  pthread_mutex_init(&q->q_lock, NULL);
  return q;
}

/*
Fill the high-price, medium-price, and low-price queues
N Customers will show up at random times during the hour, each seller will have N Customers.
*/
void enqueue(queue *q, int customer_id)
{
  customer *in_customer = malloc(sizeof(customer));
  in_customer->id = customer_id;
  in_customer->next = NULL;

  // insert the customer
  pthread_mutex_lock(&q->q_lock);
  // if empty, initialize front and rear to customer
  printf("Customer has arrived at tail of %s\n\n", q->s_id);
  if (q->rear == NULL)
  {
    q->front = q->rear = in_customer;
  }
  else
  {
    // append to end
    q->rear->next = in_customer;
    q->rear = in_customer;
  }
  q->size++;
  pthread_mutex_unlock(&q->q_lock);
}

void dequeue(queue *q)
{
  pthread_mutex_lock(&q->q_lock);

  // if not a valid list to remove from, return 
  if (q->rear == NULL || q->front == NULL)
  {
    pthread_mutex_unlock(&q->q_lock);
    return;
  }

  customer* temp = q->front;
  q->front = q->front->next;

  // if the queue is now empty, reset both
  if (q->front == NULL)
  {
    q->rear = NULL;
  }

  q->size--;
  pthread_mutex_unlock(&q->q_lock);
}

void wakeup_all_seller_threads()
{
  pthread_mutex_lock(&mutex);

  /* Ensure that all threads are waiting */
  while (done < 10)
  {
    pthread_cond_wait(&all_done, &mutex);
  }
  done = 0;

  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Please supply a second commmand line param\n\n");
    return -1;
  }

  int i;
  pthread_t tids[10];
  char *seller_type;

  seller_type = "H";
  pthread_create(&tids[0], NULL, sell, seller_type);

  seller_type = "M";
  for (i = 1; i < 4; i++)
  {
    pthread_create(&tids[i], NULL, sell, seller_type);
  }

  seller_type = "L";
  for (i = 4; i < 10; i++)
  {
    pthread_create(&tids[i], NULL, sell, seller_type);
  }

  wakeup_all_seller_threads();

  for (i = 0; i < 10; i++)
  {
    pthread_join(tids[i], NULL);
  }
}
