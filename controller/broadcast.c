#include <stdio.h>
#include <stdlib.h>
#include "sockhelp.h"
#include "thread.h"
#include "broadcast.h"

#define GLOBAL_CONTROLLER_PORT 4567

static int controllers_sockets[MAX_CONTROLLERS];
static int last_controller_socket = 0;

static callback_struct callbacks[MAX_CALLBACKS];
static int last_callback = 0;


static __thread unsigned char bcast_buff[MAX_MESSAGE];
static __thread bool new_bcast_message = false;
__thread fd_set socks;
__thread int highsock;

void build_select_list(){
	int index;
	
	FD_ZERO(&socks);
	
	for(index = 0; index < MAX_CONTROLLERS; index++)
		if(controllers_sockets[index] != 0){
			FD_SET(controllers_sockets[index],&socks);
			if(controllers_sockets[index] > highsock)
				highsock = controllers_sockets[index];
		}
}

static void do_bcast(void) {
	int index;
	int transferred_bytes;
	for(index = 0; index < last_controller_socket; index++){
		transferred_bytes = sock_write(controllers_sockets[index],bcast_buff,MAX_MESSAGE);
		if(transferred_bytes < 0){
			perror("do_bcast");
		}
	}
	new_bcast_message = false;
	// loop su tutte le socket in controllers_sockets
}

static void call_function(int sock, msg_struct msg){
	int index;
	for(index = 0; index < MAX_CALLBACKS; index++){
		if(msg.type == callbacks[index].type){
			callbacks[index].callback(sock,msg.payload,msg.size);
			return;
		}
	}
}

static void *broadcast_loop(void *args) {
	(void)args;

	int readsocks;
	int transferred_bytes;
	int index;
	struct timeval timeout;
	msg_struct msg;
	msg.payload = malloc(SIZE_PAYLOAD);
	
	// Come forwarder. In più uno switch case sul tipo di messaggio ricevuto
	while(1){
		build_select_list();
		
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
		
		if (readsocks < 0) {
			perror("select");
            exit(EXIT_FAILURE);
        }
        if (readsocks == 0) {
			printf("select - Timeout expired");
			if(new_bcast_message)
				do_bcast();
        } else {
			for(index = 0; index < last_controller_socket; index++){
				if(FD_ISSET(controllers_sockets[index],&socks)){
					transferred_bytes = sock_read(controllers_sockets[index],&msg,sizeof(msg_struct));
					if(transferred_bytes < 0){
						perror("read");
					}
					else if(transferred_bytes == 0){
						perror("read");
					}
					call_function(controllers_sockets[index],msg);
				}
				memset(msg.payload,0,SIZE_PAYLOAD);
			}
		}
		
	}
	// Quando mi sveglio dal timeout di select, controllo se new_bcast_message == true, in quel caso mando a tutti il messaggio
	// chiamando do_bcast();
}

void initialize_broadcast(const char *controllers_path) {
	
	printf("Sono nell'init del broadcast\n");
	
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
			controllers_sockets[i] = make_connection(GLOBAL_CONTROLLER_PORT, SOCK_DGRAM, line);
		}while(controllers_sockets[i] < 0);
		i++;
	}
	last_controller_socket = i;

	create_thread(broadcast_loop, NULL);

	// Registrare qui un set di function pointer associati ad un tipo di messaggio, come da callback_struct
}


void broadcast(int type, void *payload, size_t size) {

	if(size > MAX_MESSAGE) {
		fprintf(stderr, "%s:%d Sending a message too large\n", __FILE__, __LINE__);
		abort();
	}

	memcpy(bcast_buff, payload, size);

	new_bcast_message = true;
}

void register_callback(int type, void (*f)(int sock, void *content, size_t size)) {
	callbacks[last_callback].type = type;
	callbacks[last_callback++].callback = f;
}
