#include <stdio.h>
#include <stdlib.h>
#include "sockhelp.h"
#include "thread.h"
#include "broadcast.h"
#include <fcntl.h>

#define GLOBAL_CONTROLLER_PORT 	4567
#define NOT_AVAILABLE 			-71

static int controllers_sockets[MAX_CONTROLLERS];
static int last_controller_socket = 0;

static callback_struct callbacks[MAX_CALLBACKS];
static int last_callback = 0;

msg_struct msg_temp;
int * flag_first_connection;
int sock_dgram;


//static __thread unsigned char bcast_buff[MAX_MESSAGE];
//static __thread bool new_bcast_message = false;
bool new_bcast_message = false;
unsigned char bcast_buff[MAX_MESSAGE];
__thread fd_set socks;
//__thread fd_set write_socks;
__thread int highsock;

// Make a socket non-blocking
void setnonblocking(int sock) {
	int opts;

	opts = fcntl(sock, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}
	return;
}

void build_select_list(){
	int index;
	
	FD_ZERO(&socks);
	
	/*for(index = 0; index < MAX_CONTROLLERS; index++){
		if(controllers_sockets[index] != 0){
			FD_SET(controllers_sockets[index],&socks);
			if(controllers_sockets[index] > highsock)
				highsock = controllers_sockets[index];
		}
	}*/
	
	FD_SET(sock_dgram,&socks);
	if(sock_dgram > highsock) highsock = sock_dgram;
}


// This function implements the actual transfer of messages to all controllers which
// were registered at startup.
static void do_bcast(void) {
	
	printf("I'm inside do_bcast!\n");
	
	
	/*char * ack;
	ack = (char *)malloc(sizeof(char));*/
	
	int index;
	int transferred_bytes;
	for(index = 0; index < last_controller_socket; index++){
		//transferred_bytes = sock_write(controllers_sockets[index],bcast_buff,MAX_MESSAGE);
		//do{
			transferred_bytes = sock_write(controllers_sockets[index],&msg_temp,sizeof(msg_struct));
			printf("msg_temp.payload: %ld\n", *(long *)msg_temp.payload);
			printf("Sent %d bytes on socket %d\n", transferred_bytes, controllers_sockets[index]);
			if(transferred_bytes < 0){
				perror("do_bcast - sock_write");
			}
			/*printf("FLAG_FIRST_CONNECTION[%d] = %d\n", index, flag_first_connection[index]);
			if(!flag_first_connection[index]){
				printf("This is my first connection towards socket %d\n", controllers_sockets[index]);
				transferred_bytes = sock_read(controllers_sockets[index],ack,sizeof(char));
			}
			if(transferred_bytes < 0 && transferred_bytes != NOT_AVAILABLE){
				perror("do_bcast - sock_read");
			}
		}while(transferred_bytes == NOT_AVAILABLE && !flag_first_connection[index]);*/
		/*if(!flag_first_connection[index] && !strcmp(ack,"k")){
			printf("Received hack from socket %d\n", controllers_sockets[index]);
			flag_first_connection[index] = 1;
		}*/
	}
	new_bcast_message = false;
	//free(ack);
	// loop su tutte le socket in controllers_sockets
	/*
	int index;
	int writesock;
	int transferred_bytes;
	struct timeval timeout;
	
	while(1){
		
		build_select_list(write_socks);
		
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		writesock = select(highsock + 1, (fd_set *) 0, &write_socks, (fd_set *) 0, &timeout);
		
		if(writesock < 0){
			perror("do_bcast select");
		}
		
		if(writesock == 0){
			printf("do_bcast select: Timeout expired\n");
		}
		
		else{
			for(index = 0; index < last_controller_socket; index++){
				if(!FD_ISSET(controllers_sockets[index],&write_socks)){
					printf("controllers_sockets[%d] = %d non e' nel fd_set\n", index, controllers_sockets[index]);
				}
				if(FD_ISSET(controllers_sockets[index],&write_socks)){
					transferred_bytes = sock_write(controllers_sockets[index],&msg_temp,sizeof(msg_struct));
					if(transferred_bytes < 0){
						perror("write_less_than_0");
					}
					else if(transferred_bytes == 0){
						perror("write_equal_to_0");
					}
					printf("Sono la socket %d ed ho mandato il messaggio con tipo: %d\n", controllers_sockets[index],msg_temp.type);
				}
			}
		}
		new_bcast_message = false;
	}*/
}


// This is the entry point of the demultiplexer of callback functions
// registered to manage messages which are received from controllers as
// broadcast functions.
static void call_function(int sock, msg_struct msg){
	
	printf("I'm inside call function\n");
	
	int index;
	for(index = 0; index < MAX_CALLBACKS; index++){
		if(msg.type == 0) continue;
		if(msg.type == callbacks[index].type){
			printf("in if of call_funcion\n");
			printf("Value is: %s\n", (char *) callbacks[index].callback);
			callbacks[index].callback(sock,&msg.payload,msg.size);
			return;
		}
	}
}


// This is the main thread which manages the reception of information
// from remote broadcasts
static void *broadcast_loop(void *args) {
	(void)args;

	int readsocks;
	int transferred_bytes;
	int index;
	struct timeval timeout;
	msg_struct msg;
	msg.payload = malloc(SIZE_PAYLOAD);
	
	bool my_new_bcast_message;
	
	// as the forwarder's code. We switch on message type
	while(1){
		
		build_select_list();
		
		my_new_bcast_message = new_bcast_message;
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		printf("SONO PRIMA DELLA SELECT\n");
		readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
		printf("SONO DOPO DELLA SELECT\n");
		if (readsocks < 0) {
			perror("select");
            exit(EXIT_FAILURE);
        }
        if (readsocks == 0) {
			printf("select - Timeout expired\n");
			if(my_new_bcast_message)
				do_bcast();
        } else {
			if(FD_ISSET(sock_dgram,&socks)){
				transferred_bytes = sock_read(sock_dgram,&msg,sizeof(msg_struct));
				if(transferred_bytes < 0){
					perror("read_less_than_0");
				}
				printf("Sono la socket %d ed ho ricevuto il messaggio con tipo: %d\n", sock_dgram,msg.type);
				
				call_function(controllers_sockets[0],msg);
				
				//call_function(controllers_sockets[index],msg);
			}
			
			//memset(msg.payload,0,sizeof(void *));
			
			/*for(index = 0; index < last_controller_socket; index++){
				if(!FD_ISSET(controllers_sockets[index],&socks)){
					printf("controllers_sockets[%d] = %d non e' nel fd_set\n", index, controllers_sockets[index]);
				}
				if(FD_ISSET(controllers_sockets[index],&socks)){
					transferred_bytes = sock_read(controllers_sockets[index],&msg,sizeof(msg_struct));
					if(transferred_bytes < 0){
						perror("read_less_than_0");
					}
					else if(transferred_bytes == 0){
						perror("read_equal_to_0");
					}*/
					
					/*if(!flag_first_connection[index]){
						printf("It's my first connection, reply to ack!\n");
						char * ack_r;
						ack_r = (char *)malloc(sizeof(char));
						strcpy(ack_r,"k");
						transferred_bytes = sock_write(controllers_sockets[index],ack_r,sizeof(char));
						if(transferred_bytes < 0){
							perror("broadcast_loop - sock_write");
						}
					}*/
					/*printf("Sono la socket %d ed ho ricevuto il messaggio con tipo: %d\n", controllers_sockets[index],msg.type);
					call_function(controllers_sockets[index],msg);
				}
				memset(msg.payload,0,SIZE_PAYLOAD);
			}*/
		}
		
	}
}

void start_server_dgram(int * sockfd){
	int sock;
	struct sockaddr_in temp;
	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("start_server_dgram - socket");
		exit(1);
	}
	
	temp.sin_family = AF_INET;
	temp.sin_addr.s_addr = htonl(INADDR_ANY);
	temp.sin_port = htons((int)GLOBAL_CONTROLLER_PORT);
	
	if(bind(sock,(struct sockaddr *) &temp, sizeof(temp)) <0){
		perror("start_server_dgram - bind");
		exit(1);
	}
	/*
	char buffer[4096];
	struct sockaddr_in client;
	unsigned int addr_len;
	addr_len = sizeof(struct sockaddr_in);
	int numbytes;
	while(1){
		printf("Waiting for datas...\n");
		if((numbytes = recvfrom(sock,buffer,4096,0,(struct sockaddr *)&client,&addr_len)) < 0){
			perror("recvfrom");
		}
		printf("Ho ricevuto: %s\n", buffer);
		sleep(1);
	}*/
	
	*sockfd = sock;
}

void initialize_broadcast(const char *controllers_path) {
	
	printf("Initializing broadcast...\n");
	
	start_server_dgram(&sock_dgram);
	
	FILE *f;
	char line[128];
	int i = 0;
	int index;

	f = fopen(controllers_path, "r");
	if(f == NULL) {
		perror("Unable to load the list of controllers");
		exit(EXIT_FAILURE);
	}

	while (fgets(line, 128, f) != NULL) {
		do{
			controllers_sockets[i] = make_connection(GLOBAL_CONTROLLER_PORT, SOCK_DGRAM, line);
			// serve?
			setnonblocking(controllers_sockets[i]);
		}while(controllers_sockets[i] < 0);
		printf("Connected to %s with soscket %d\n", line, controllers_sockets[i]);
		i++;
	}
	last_controller_socket = i;

	/*flag_first_connection = (int *)malloc(sizeof(int)*last_controller_socket);
	for(index = 0; index < last_controller_socket; index++){
		flag_first_connection[index] = 0;
	}*/

	create_thread(broadcast_loop, NULL);
	
	printf("Broadcast UP!\n");

	// Registrare qui un set di function pointer associati ad un tipo di messaggio, come da callback_struct
}


void broadcast(int type, void *payload, size_t size) {
	
	printf("I'm inside the broadcast function\n");

	if(size > MAX_MESSAGE) {
		fprintf(stderr, "%s:%d Sending a message too large\n", __FILE__, __LINE__);
		abort();
	}
	//msg_temp.payload = malloc(SIZE_PAYLOAD);
	msg_temp.payload = malloc(sizeof(void *));
	msg_temp.type = type;
	memcpy(msg_temp.payload,payload,size);
	msg_temp.size = size;
	
	//memcpy(bcast_buff, payload, size);

	new_bcast_message = true;
}

void register_callback(int type, void (*f)(int sock, void *content, size_t size)) {
	callbacks[last_callback].type = type;
	callbacks[last_callback++].callback = f;
}
