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
int available_seats = NUM_SEATS;

pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t minute_cond = PTHREAD_COND_INITIALIZER;
int minute = 0;

int customer_id = 0;
typedef struct customer
{
  int id;
  customer *next;
} customer;

typedef struct seller
{
  char *s_id;
  int seller_type;
  int seller_number;
  statistics stats;
} seller;
// Queue using a LL implementation
typedef struct queue
{
  customer *front;
  customer *rear;
  int size;
  seller *seller_info;
  pthread_mutex_t q_lock;
} queue;

typedef struct statistics
{
  float averageResponseTime;
  float averageTurnaroundTime;
  float averageThroughput;
} statistics;

// We need the aggregate response time, turnaround, and throughput for each type of seller later.
queue *h_price;
queue *m_price[3];
queue *l_price[6];

int get_sell_times(int seller_type)
{
  switch (seller_type)
  {
  case 0:
    return 1 + rand() % 2; // H price
  case 1:
    return 2 + rand() % 3; // M price
  case 2:
    return 3 + rand() % 4; // L price
  }
}

queue *setup_queue(char *seller_name)
{
  queue *q = malloc(sizeof(queue));
  q->seller_info = malloc(sizeof(seller));
  q->seller_info->s_id = malloc(strlen(seller_name) + 2);
  strcpy(q->seller_info->s_id, seller_name);
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
  printf("Customer has arrived at tail of %s\n\n", q->seller_info->s_id);
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

// Reason == 0: Customer gets a ticket, Reason == 1: Concert sold out, Reason == 2:
customer *dequeue(queue *q)
{
  pthread_mutex_lock(&q->q_lock);

  // if not a valid list to remove from, return
  if (q->rear == NULL || q->front == NULL)
  {
    pthread_mutex_unlock(&q->q_lock);
    return;
  }

  customer *temp = q->front;
  q->front = q->front->next;

  // if the queue is now empty, reset both
  if (q->front == NULL)
  {
    q->rear = NULL;
  }

  q->size--;

  pthread_mutex_unlock(&q->q_lock);
  return temp;
}

void init_sellers()
{
  h_price = setup_queue("H1");
  h_price->seller_info->seller_type = 0;
  h_price->seller_info->seller_number = 0;
  char seller_name[4];

  for (int i = 0; i < 3; i++)
  {
    l_price[i] = setup_queue("M");
    sprintf(seller_name, "M%d", i + 1);
    m_price[i] = setup_queue(seller_name);
    m_price[i]->seller_info->seller_type = 1;
    m_price[i]->seller_info->seller_number = i;
  }
  for (int i = 0; i < 6; i++)
  {
    l_price[i] = setup_queue("L");
    sprintf(seller_name, "L%d", i + 1);
    l_price[i] = setup_queue(seller_name);
    l_price[i]->seller_info->seller_type = 2;
    l_price[i]->seller_info->seller_number = i;
  }
}

void *sell(void *p_seller)
{
  seller *curr_seller = (seller *)p_seller;
  queue *curr_queue = NULL;

  if (curr_seller->seller_type == 0)
  {
    curr_queue = h_price;
  }
  else if (curr_seller->seller_type == 1)
  {
    curr_queue = m_price[curr_seller->seller_number];
  }
  else
  {
    curr_queue = l_price[curr_seller->seller_number];
  }

  pthread_mutex_lock(&mutex);
  done++;
  if (done == 10)
  {
    pthread_cond_signal(&all_done);
  }
  pthread_cond_wait(&cond, &mutex);

  pthread_mutex_unlock(&mutex);

  while (minute < HOUR)
  {
    customer *curr_customer = dequeue(curr_queue);
    int start_time = minute;
    if (curr_customer != NULL)
    {
      // Assign a seat HERE
      int seat_number = 0;

      if (seat_number != -1)
      {
        printf("at %d: Seller %d sells a ticket assigned to seat %d\n", minute, curr_seller->s_id, seat_number);
        int sell_time = get_sell_times(curr_seller->seller_type);

        for (int i = 0; i < sell_time; i++)
        {
          pthread_mutex_lock(&time_mutex);
          // let them sell beyond 1 hour
          while (minute < start_time + i + 1)
          {
            pthread_cond_wait(&minute_cond, &time_mutex);
          }
          pthread_mutex_unlock(&time_mutex);
        }
      }
      else
      {
        printf("at %d: Customer leaves as concert is sold out.");
      }
      free(curr_customer);
    }
    pthread_mutex_lock(&time_mutex);
    pthread_cond_wait(&minute_cond, &time_mutex);
    pthread_mutex_unlock(&time_mutex);
  }
}

void *clock()
{
  while (minute < HOUR)
  {
    sleep(1);
    pthread_mutex_lock(&time_mutex);
    minute++;
    pthread_cond_broadcast(&minute_cond);
    pthread_mutex_unlock(&time_mutex);
  }
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

void generate_customers(int N)
{
  srand(time(NULL));
  for (int i = 0; i < 10; i++)
  {
    queue *curr_queue = NULL;

    if (i == 0)
    {
      curr_queue = h_price;
    }
    else if (i < 4)
    {
      curr_queue = m_price[i - 1];
    }
    else
    {
      curr_queue = l_price[i - 4];
    }

    for (int j = 0; j < N; j++)
    {
      customer *n_customer = malloc(sizeof(customer));
      n_customer->id = customer_id++;
      n_customer->next = NULL;
      enqueue(curr_queue, n_customer->id);
    }
  }
}

void print_stats()
{
  /*** TODO */
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Please supply a second commmand line param\n\n");
    return -1;
  }

  init_sellers();
  // at some random time -- generate_customers()
  print_stats();
  return 1;
}
