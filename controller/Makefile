CFLAGS=-g

all:
	gcc -g -pthread server.c ml_model.c sockhelp.c flow_function.c -o controller
	gcc -g -pthread balancer.c atomic.c sockhelp.c thread.c flow_function.c -o balancer
	gcc -g client.c -o client

clean:
	-rm controller client balancer
