CC=gcc
CFLAGS=-lpthread

m_TicketSellers: m_TicketSellers.o 
	$(CC) -o ticketSellers m_TicketSellers.o $(CFLAGS)

m_TicketSellers.o: m_TicketSellers.c 
	$(CC) -c m_TicketSellers.c $(CFLAGS)

clean:
	rm -f *.o ticketSellers