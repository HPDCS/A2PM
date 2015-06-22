CFLAGS=-g

all:
	gcc -pthread server.c ml_model.c sockhelp.c -o controller
	gcc -pthread balancer.c atomic.c sockhelp.c thread.c -o balancer
	gcc client.c -o client

clean:
	-rm controller client balancer
