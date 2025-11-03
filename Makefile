CC=gcc
OBJS = main.o random_pick.o lru.o lfu.o fifo.o mfu.o

simulation: $(OBJS)
	$(CC) $(CFLAGS) -o simulation $(OBJS)

main.o: main.c common.h
	$(CC) $(CFLAGS) -c main.c

random_pick.o: random_pick.c common.h
	$(CC) $(CFLAGS) -c random_pick.c

lru.o: lru.c common.h
	$(CC) $(CFLAGS) -c lru.c

lfu.o: lfu.c common.h
	$(CC) $(CFLAGS) -c lfu.c

fifo.o: fifo.c common.h
	$(CC) $(CFLAGS) -c fifo.c

mfu.o: mfu.c common.h
	$(CC) $(CFLAGS) -c mfu.c

clean:
	rm -f *.o simulation