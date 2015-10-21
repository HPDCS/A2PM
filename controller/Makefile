CFLAGS=-g

all:
	gcc -g -pthread controller.c ml_model.c vm_list.c sockhelp.c flow_function.c -o controller
	gcc -g -pthread balancer.c atomic.c vm_list.c sockhelp.c thread.c flow_function.c -o balancer
	gcc -g -pthread server.c -o server
	gcc -g -pthread user.c -o user
	gcc -g client.c -o client

clean:
	-rm controller client balancer
