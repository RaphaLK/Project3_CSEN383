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
#include <string.h>
#include <time.h>
#include <unistd.h>

#define HOUR 60
#define NUM_SEATS 100
#define ROWS 10
#define SEATS_PER_ROW 10

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
  int arrival_time;
  int sell_start_time;
  int sell_end_time;
  struct customer *next;
  struct queue *seller_queue;
} customer;

customer *customer_arrival_list = NULL;
pthread_mutex_t arrival_list_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct statistics
{
  float averageResponseTime;
  float averageTurnaroundTime;
  float averageThroughput;
} statistics;
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

typedef struct seat
{
  int seat_number;
  int customer_id;
} seat;

// Track next available position for each seller type to avoid redundant scanning
// Note: These cursors are protected by seats_mutex (not separate mutexes)
// since cursor updates always happen atomically with seat assignments
typedef struct seat_cursor
{
  int row_index;  // Current position in the row order
  int seat_index; // Current seat within the row
} seat_cursor;

seat_cursor h_cursor = {0, 0}; // H sellers: rows 0->9
seat_cursor m_cursor = {0, 0}; // M sellers: rows 4,5,3,6,2,7,1,8,0,9 (shared by 3 sellers)
seat_cursor l_cursor = {0, 0}; // L sellers: rows 9->0 (shared by 6 sellers)

pthread_mutex_t seats_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects seat array AND cursors
seat *all_seats[ROWS][SEATS_PER_ROW];

void init_seats()
{
  int seat_number = 0;
  for (int r = 0; r < ROWS; r++)
  {
    for (int s = 0; s < SEATS_PER_ROW; s++)
    {
      all_seats[r][s] = malloc(sizeof(seat));
      all_seats[r][s]->seat_number = seat_number;
      all_seats[r][s]->customer_id = -1;
      seat_number++;
    }
  }
}

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
void enqueue(queue *q, customer *in_customer)
{
  in_customer->next = NULL;

  // insert the customer
  pthread_mutex_lock(&q->q_lock);
  // if empty, initialize front and rear to customer
  printf("at %d: Customer %d has arrived at tail of %s.\n", minute, in_customer->id, q->seller_info->s_id);
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
    return NULL;
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
    sprintf(seller_name, "M%d", i + 1);
    m_price[i] = setup_queue(seller_name);
    m_price[i]->seller_info->seller_type = 1;
    m_price[i]->seller_info->seller_number = i;
  }
  for (int i = 0; i < 6; i++)
  {
    sprintf(seller_name, "L%d", i + 1);
    l_price[i] = setup_queue(seller_name);
    l_price[i]->seller_info->seller_type = 2;
    l_price[i]->seller_info->seller_number = i;
  }
}

/**
 * Assign seat to customer after checking availability
 */
int assign_seat(int row, int seat, customer *customer)
{
  all_seats[row][seat]->customer_id = customer->id;
  available_seats--;
  return all_seats[row][seat]->seat_number;
}

/**
 * Updates cursor for the next seat position after assigning a seat
 */
void update_cursor(seat_cursor *cursor, int current_row_index, int current_seat)
{
  cursor->seat_index = current_seat + 1;
  if (cursor->seat_index >= SEATS_PER_ROW)
  {
    cursor->seat_index = 0;
    cursor->row_index = current_row_index + 1;
  }
  else
  {
    cursor->row_index = current_row_index;
  }
}

/**
 * Seat assignment by High-priced seller (order: rows 1 -> 10)
 */
int h_assign_seats(customer *customer)
{
  pthread_mutex_lock(&seats_mutex);
  if (available_seats > 0)
  {
    // Start from cursor position
    for (int r = h_cursor.row_index; r < ROWS; r++)
    {
      int start_seat = (r == h_cursor.row_index) ? h_cursor.seat_index : 0;
      for (int s = start_seat; s < SEATS_PER_ROW; s++)
      {
        if (all_seats[r][s]->customer_id == -1)
        {
          int seat_number = assign_seat(r, s, customer);
          update_cursor(&h_cursor, r, s);

          pthread_mutex_unlock(&seats_mutex);
          return seat_number;
        }
      }
    }
  }

  pthread_mutex_unlock(&seats_mutex);
  return -1;
}

/**
 * Seat assignment by Medium-priced seller (order: row 5, 6, 4, 7, 3, 8, 2, 9, 1, 10)
 */
int m_assign_seats(customer *customer)
{
  static const int order[ROWS] = {4, 5, 3, 6, 2, 7, 1, 8, 0, 9};
  pthread_mutex_lock(&seats_mutex);
  if (available_seats > 0)
  {
    // Start from cursor position
    for (int i = m_cursor.row_index; i < ROWS; i++)
    {
      int r = order[i]; // Map i to actual index of the row to scan
      int start_seat = (i == m_cursor.row_index) ? m_cursor.seat_index : 0;

      for (int s = start_seat; s < SEATS_PER_ROW; s++)
      {
        if (all_seats[r][s]->customer_id == -1)
        {
          int seat_number = assign_seat(r, s, customer);
          update_cursor(&m_cursor, i, s);

          pthread_mutex_unlock(&seats_mutex);
          return seat_number;
        }
      }
    }
  }

  pthread_mutex_unlock(&seats_mutex);
  return -1;
}

/**
 * Seat assignment by Low-priced seller (order: rows 10 -> 1)
 */
int l_assign_seats(customer *customer)
{
  pthread_mutex_lock(&seats_mutex);
  if (available_seats > 0)
  {
    // Start from cursor position
    for (int i = l_cursor.row_index; i < ROWS; i++)
    {
      int r = ROWS - 1 - i; // Map i to actual index of the row to scan
      int start_seat = (i == l_cursor.row_index) ? l_cursor.seat_index : 0;
      for (int s = start_seat; s < SEATS_PER_ROW; s++)
      {
        if (all_seats[r][s]->customer_id == -1)
        {
          int seat_number = assign_seat(r, s, customer);
          update_cursor(&l_cursor, i, s);

          pthread_mutex_unlock(&seats_mutex);
          return seat_number;
        }
      }
    }
  }

  pthread_mutex_unlock(&seats_mutex);
  return -1;
}

/**
 * Assign seat based on seller type
 */
int assign_by_seller_type(customer *customer, seller *seller)
{
  switch (seller->seller_type)
  {
  case 0:
    return h_assign_seats(customer);
  case 1:
    return m_assign_seats(customer);
  default:
    return l_assign_seats(customer);
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

  // Synchronization Barrier
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
      int seat_number = assign_by_seller_type(curr_customer, curr_seller);

      if (seat_number != -1)
      {
        printf("at %d: Seller %s sells a ticket to: %d assigned to seat %d\n", minute, curr_seller->s_id, curr_customer->id, seat_number);
        int sell_time = get_sell_times(curr_seller->seller_type);

        for (int i = 0; i < sell_time; i++)
        {
          pthread_mutex_lock(&time_mutex);
          // let them sell beyond 1 hour
          while (minute < start_time + i + 1 && minute < HOUR)
          {
            pthread_cond_wait(&minute_cond, &time_mutex);
          }
          pthread_mutex_unlock(&time_mutex);
        }
      }
      else
      {
        printf("at %d: Customer leaves as concert is sold out.\n", minute);
      }
      free(curr_customer);
    }
    else
    {
      // No customer in queue, wait for next minute or until program ends
      pthread_mutex_lock(&time_mutex);
      if (minute < HOUR) {
        pthread_cond_wait(&minute_cond, &time_mutex);
      }
      pthread_mutex_unlock(&time_mutex);
    }
  }
}

void *sell_clock()
{
  while (minute < HOUR)
  {
    sleep(1);
    pthread_mutex_lock(&time_mutex);
    minute++;
    pthread_cond_broadcast(&minute_cond);
    pthread_mutex_unlock(&time_mutex);

    // Check if all queues are empty
    int empty_check = 1;
    pthread_mutex_lock(&h_price->q_lock);
    if (h_price->size > 0) empty_check = 0;
    pthread_mutex_unlock(&h_price->q_lock);
    
    for (int i = 0; i < 3 && empty_check; i++) {
      pthread_mutex_lock(&m_price[i]->q_lock);
      if (m_price[i]->size > 0) empty_check = 0;
      pthread_mutex_unlock(&m_price[i]->q_lock);
    }
    
    for (int i = 0; i < 6 && empty_check; i++) {
      pthread_mutex_lock(&l_price[i]->q_lock);
      if (l_price[i]->size > 0) empty_check = 0;
      pthread_mutex_unlock(&l_price[i]->q_lock);
    }
    
    // Check da arrival queue
    pthread_mutex_lock(&arrival_list_mutex);
    if (customer_arrival_list != NULL) empty_check = 0;
    pthread_mutex_unlock(&arrival_list_mutex);
    
    if (empty_check) {
      pthread_mutex_lock(&time_mutex);
      minute = HOUR; // Set the time to max to turn off the program automatically.
      pthread_cond_broadcast(&minute_cond);
      pthread_mutex_unlock(&time_mutex);
      break;
    }
  }
  pthread_mutex_lock(&time_mutex);
  pthread_cond_broadcast(&minute_cond);
  pthread_mutex_unlock(&time_mutex);

  return NULL;
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
      n_customer->arrival_time = rand() % HOUR;
      n_customer->seller_queue = curr_queue;
      pthread_mutex_lock(&arrival_list_mutex);

      if (customer_arrival_list == NULL || customer_arrival_list->arrival_time > n_customer->arrival_time)
      {
        n_customer->next = customer_arrival_list;
        customer_arrival_list = n_customer;
      }
      else
      {
        customer *curr = customer_arrival_list;
        while (curr->next != NULL && curr->next->arrival_time <= n_customer->arrival_time)
        {
          curr = curr->next;
        }
        n_customer->next = curr->next;
        curr->next = n_customer;
      }
      pthread_mutex_unlock(&arrival_list_mutex);
    }
  }
}

void *customer_arrival()
{
  while (minute < HOUR)
  {
    pthread_mutex_lock(&arrival_list_mutex);
    while (customer_arrival_list != NULL && customer_arrival_list->arrival_time <= minute)
    {
      // Remove from arrival list
      customer *arrived_customer = customer_arrival_list;
      customer_arrival_list = customer_arrival_list->next;

      // Enqueue to the target seller's queue
      enqueue(arrived_customer->seller_queue, arrived_customer);
    }
    pthread_mutex_unlock(&arrival_list_mutex);

    // Wait for the next minute
    pthread_mutex_lock(&time_mutex);
    if (minute < HOUR)
    {
      pthread_cond_wait(&minute_cond, &time_mutex);
    }
    pthread_mutex_unlock(&time_mutex);
  }
  return NULL;
}

void print_stats()
{
  /*** TODO */
  return;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Please supply a second commmand line param\n\n");
    return -1;
  }
  pthread_t seller_threads[10];
  pthread_t clock_thread, customer_thread;

  init_seats();
  init_sellers();
  // at some random time -- generate_customers()
  generate_customers(atoi(argv[1]));

  pthread_create(&seller_threads[0], NULL, sell, (void *)h_price->seller_info);
  for (int i = 0; i < 3; i++)
  {
    pthread_create(&seller_threads[i + 1], NULL, sell, (void *)m_price[i]->seller_info);
  }
  for (int i = 0; i < 6; i++)
  {
    pthread_create(&seller_threads[i + 4], NULL, sell, (void *)l_price[i]->seller_info);
  }

  // Create customer arrival and clock threads
  pthread_create(&customer_thread, NULL, customer_arrival, NULL);
  pthread_create(&clock_thread, NULL, sell_clock, NULL);

  // Start simulation
  wakeup_all_seller_threads();

  // Wait for threads to finish
  for (int i = 0; i < 10; i++)
  {
    pthread_join(seller_threads[i], NULL);
  }
  pthread_join(clock_thread, NULL);
  pthread_join(customer_thread, NULL);
  print_stats();
  return 1;
}
