#include <stdio.h>
#include <stdlib.h>
#include "sockhelp.h"
#include "thread.h"
#include "broadcast.h"
#include <fcntl.h>

#define GLOBAL_CONTROLLER_PORT 	4567

static int controllers_sockets[MAX_CONTROLLERS];
static int last_controller_socket = 0;

static callback_struct callbacks[MAX_CALLBACKS];
static int last_callback = 0;

msg_struct msg_temp;
int sock_udp;

struct ip_socket_receiver{
	char ip[16];
	int socket;
	long value;
};

struct ip_socket_receiver connections[MAX_CONTROLLERS];

bool new_bcast_message = false;
unsigned char bcast_buff[MAX_MESSAGE];
__thread fd_set socks;
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
	FD_SET(sock_udp,&socks);
	if(sock_udp > highsock) highsock = sock_udp;
}


// This function implements the actual transfer of messages to all controllers which
// were registered at startup.
static void do_bcast(void) {
	struct sockaddr_in client;
	int index;
	int transferred_bytes;
	
	for(index = 0; index < last_controller_socket; index++){
		transferred_bytes = sock_write_udp(controllers_sockets[index],&msg_temp,sizeof(msg_struct),connections[index].ip);
		printf("Sent %d bytes on socket %d\n", transferred_bytes, controllers_sockets[index]);
		if(transferred_bytes < 0){
			perror("do_bcast - sock_write_udp");
		}
	}
	new_bcast_message = false;
}


// This is the entry point of the demultiplexer of callback functions
// registered to manage messages which are received from controllers as
// broadcast functions.
static void call_function(int sock, msg_struct msg){
	int index;
	for(index = 0; index < MAX_CALLBACKS; index++){
		if(msg.type == 0) continue;
		if(msg.type == callbacks[index].type){
			printf("in if of call_funcion\n");
			printf("Value is: %s\n", (char *) callbacks[index].callback);
			callbacks[index].callback(sock,msg.payload,msg.size);
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
	//msg.payload = malloc(SIZE_PAYLOAD);
	
	bool my_new_bcast_message;
	
	// as the forwarder's code. We switch on message type
	while(1){
		
		build_select_list();
		
		my_new_bcast_message = new_bcast_message;
		
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
		if (readsocks < 0) {
			perror("select");
            exit(EXIT_FAILURE);
        }
        if (readsocks == 0) {
			printf("select - Timeout expired\n");
			if(my_new_bcast_message)
				do_bcast();
        } else {
			if(FD_ISSET(sock_udp,&socks)){
				
				//transferred_bytes = sock_read_udp(sock_udp,&msg,sizeof(msg_struct));
				struct sockaddr_in receiver;
				unsigned int size_receiver = sizeof(receiver);
				transferred_bytes = recvfrom(sock_udp, &msg, sizeof(msg_struct), 0, (struct sockaddr *)&receiver, &size_receiver);
				printf("Received the value %ld from controller %s\n", msg.payload, inet_ntoa(receiver.sin_addr));
				if(transferred_bytes < 0){
					perror("read_less_than_0");
				}
				
				int i;
				for(i = 0; i < last_controller_socket; i++){
					if(!strcmp(connections[i].ip, inet_ntoa(receiver.sin_addr))){
						connections[i].value = msg.payload;
						break;
					}
				}
				/* CAMBIARE QUANDO METTERO' PIU' CONTROLLER!!! */
				//call_function(controllers_sockets[0],msg);
				call_function(connections[i].socket,msg);
			}
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
	*sockfd = sock;
}

void initialize_broadcast(const char *controllers_path) {
	start_server_dgram(&sock_udp);
	
	FILE *f;
	char line[128];
	int i = 0;

	f = fopen(controllers_path, "r");
	if(f == NULL) {
		perror("Unable to load the list of controllers");
		exit(EXIT_FAILURE);
	}

	while (fgets(line, 128, f) != NULL) {
		do{
			connections[i].socket = make_connection(GLOBAL_CONTROLLER_PORT, SOCK_DGRAM, line);
			controllers_sockets[i] = connections[i].socket;
			strncpy(connections[i].ip,line,strlen(line)-1);
		}while(connections[i].socket < 0);
		printf("BROADCAST - Connected to %s with soscket %d\n", line, connections[i].socket);
		i++;
	}
	last_controller_socket = i;

	create_thread(broadcast_loop, NULL);
}

void broadcast(int type, long payload, size_t size) {
//void broadcast(int type, void *payload, size_t size) {

	if(size > MAX_MESSAGE) {
		fprintf(stderr, "%s:%d Sending a message too large\n", __FILE__, __LINE__);
		abort();
	}
	//msg_temp.payload = malloc(SIZE_PAYLOAD);
	//msg_temp.payload = malloc(sizeof(void *));
	msg_temp.type = type;
	msg_temp.payload = payload;
	//memcpy(msg_temp.payload,payload,size);
	msg_temp.size = size;
	
	//memcpy(bcast_buff, payload, size);

	new_bcast_message = true;
}

//void register_callback(int type, void (*f)(int sock, void *content, size_t size)) {
void register_callback(int type, void (*f)(int sock, long content, size_t size)) {
	callbacks[last_callback].type = type;
	callbacks[last_callback++].callback = f;
}
