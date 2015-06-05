CFLAGS=-g

all:
	$(CC) client.c $(CFLAGS) -o controller_client
	$(CC) server.c ml_model.c leader.c broadcast.c thread.c atomic.c sockhelp.c $(CFLAGS) -o controller_server -lpthread
	$(CC) balancer.c sockhelp.c thread.c atomic.c $(CFLAGS) -pthread -o balancer

clean:
	-rm controller_client controller_server balancer
