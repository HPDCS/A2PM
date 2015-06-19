#include "sockhelp.h"
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <arpa/inet.h>
#include "thread.h"


#define MAX_NUM_OF_CLIENTS		1024			//Max number of accepted clients
#define FORWARD_BUFFER_SIZE		1024*1024		//Size of buffers
#define NUMBER_VMs			1024				//It must be equal to the value in server side in controller
#define NUMBER_GROUPS			3				//It must be equal to the value in server side in controller
#define MAX_CONNECTED_CLIENTS		5				//It represents the max number of connected clients
#define NOT_AVAILABLE			-71

int current_vms[NUMBER_GROUPS];				//Number of connected VMs
int allocated_vms[NUMBER_GROUPS];			//Number of possible VMs
int index_vms[NUMBER_GROUPS];				//It represents (for each service) the VM that has to be contacted
int actual_index[NUMBER_GROUPS];			//It represents the actual index to assign VM
pthread_mutex_t mutex;
int res_thread;

//Used to pass client info to threads
struct arg_thread{
	int socket;
	char ip_address[16];
	int port;
};

//Provided services
enum services{
	SERVICE_1, SERVICE_2, SERVICE_3
};

// TODO: posso non discriminare DEL e REJ operations? Credo di si!
enum operations{
	ADD, DELETE, REJ
};

//This struct is different from the struct in the controller, because balancer needs less infos than controller
struct vm_data{
	char ip_address[16];
	int port;
	
	//It represents a list of connected clients to this particular TPCW instance
	char * connected_clients[MAX_CONNECTED_CLIENTS];
};

//System's topology representation
struct vm_data * vm_data_set[NUMBER_GROUPS];

int sock; // Listening socket
__thread void *buffer_from_client;
__thread void *buffer_to_client;
__thread void *aux_buffer_from_client;
__thread void *aux_buffer_to_client;
__thread int connectlist[2];  // One thread handles only 2 sockets
__thread fd_set socks; // Socket file descriptors we want to wake up for, using select()
__thread int highsock; //* Highest #'d file descriptor, needed for select()

int da_socket = -1;

void setnonblocking(int sock);

/*
 * check for the vm_data_set size
 */
void check_vm_data_set_size(int service){
	if(current_vms[service] == allocated_vms[service]){
		vm_data_set[service] = realloc(vm_data_set[service],sizeof(struct vm_data)*2*allocated_vms[service]);
		allocated_vms[service] *= 2;
	}
}
/*
 * delete a vm
 */

void delete_vm(char * ip_address, int service, int port){
	char ip[16];
	strcpy(ip, ip_address);
	int index;
	for(index = 0; index < current_vms[service]; index++){
		if(strcmp(vm_data_set[service][index].ip_address,ip) == 0 && vm_data_set[service][index].port == port){
			vm_data_set[service][index] = vm_data_set[service][--current_vms[service]];
			break;
		}
	}
}

// Append to the original buffer the content of aux_buffer
void append_buffer(char * original_buffer, char * aux_buffer, int * bytes_original,int bytes_aux, int * times){
	
	if((*bytes_original + bytes_aux) >= FORWARD_BUFFER_SIZE){
		printf("REALLOC: STAMPA TIMES: %d\n", *times);
		original_buffer = realloc(original_buffer, (*times)*FORWARD_BUFFER_SIZE);
		printf("HO FATTO LA REALLOC!\n");
		(*times)++;
	}
	//strncpy(&original_buffer[*bytes_original],aux_buffer,bytes_aux);
	//strncpy(&(original_buffer[*bytes_original]),aux_buffer,(FORWARD_BUFFER_SIZE - *bytes_original));
	memcpy(&(original_buffer[*bytes_original]),aux_buffer,bytes_aux);
	*bytes_original = *bytes_original + bytes_aux;
	bzero(aux_buffer, FORWARD_BUFFER_SIZE);
}

// Look for an IP address in the internal representation

int search_ip(struct vm_data * tpcw_instance, char * ip){
	int index;
	for(index = 0; index < MAX_CONNECTED_CLIENTS; index++){
		if(!strcmp(tpcw_instance->connected_clients[index],ip)){
			return ++index;
		}
	}
	return 0;
}


// Check whether a remote host has already connected to me
struct sockaddr_in check_already_connected(char * ip){
	
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	
	// TODO: this code is commented only for debug purposes, do not remove

	/*int index;
	int service;
	for(service = 0; service < NUMBER_GROUPS; service){
		for(index = 0; index < current_vms[service]; index++){
			if((search_ip(&vm_data_set[service][index],ip)) > 0){
				client.sin_addr.s_addr = inet_addr(vm_data_set[service][index].ip_address);
				client.sin_port = vm_data_set[service][index].port;
				return client;
			}
		}
	}
	
	client.sin_addr.s_addr = inet_addr(vm_data_set[service][actual_index[service]].ip_address);
	client.sin_port = vm_data_set[service][actual_index[service]].port;
	
	strcpy(vm_data_set[service][actual_index[service]].connected_clients[search_ip(&vm_data_set[service][actual_index[service]], "0.0.0.0")-1],ip);
	actual_index[service]++;
	
	return client;*/
	
	int index;
	for(index = 0; index < current_vms[0]; index++){
		if((search_ip(&vm_data_set[0][index],ip)) > 0){
			client.sin_addr.s_addr = inet_addr(vm_data_set[0][index].ip_address);
			client.sin_port = vm_data_set[0][index].port;
			return client;
		}
	}
	
	client.sin_addr.s_addr = inet_addr(vm_data_set[0][actual_index[0]].ip_address);
	client.sin_port = vm_data_set[0][actual_index[0]].port;
	
	strcpy(vm_data_set[0][actual_index[0]].connected_clients[search_ip(&vm_data_set[0][actual_index[0]], "0.0.0.0")-1],ip);
	actual_index[0]++;
	
	return client;
}

// Get the current open socket to a give client's IP
int get_current_socket(char * ip_client) {
	
	// TODO: this code is commented only for debug purposes, do not remove

	/*int index;
	for(index = 0; index < NUMBER_GROUPS; index++){
		if(actual_index[index] == current_vms[index])
			actual_index[index] = 0;
	}*/
	if(actual_index[0] == current_vms[0]) actual_index[0] = 0;
	
	da_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (da_socket < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in temp;

	temp = check_already_connected(ip_client);
	
	printf("GET_CURRENT_SOCKET --- IP: %s AND PORT: %d\n", inet_ntoa(temp.sin_addr), ntohs(temp.sin_port));
	// Connect to controller (it creates the connection LB - Controller)
	if (connect(da_socket, (struct sockaddr *)&temp , sizeof(temp)) < 0) {
		perror("get_current_socket: connect_to_controller");
		exit(EXIT_FAILURE);
	}
	printf("TPCW IP ADDRESS %s\n", inet_ntoa(temp.sin_addr));
	printf("\n\n\n*** CONNECTION ESTABLISHED WITH TPCW - SOCKET %d ***\n\n\n", da_socket);
	setnonblocking(da_socket); 
	
	return da_socket;

}

// Make an already-created socket non-blocking
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


// This creates a selection list for select()
void build_select_list() {
	int listnum; //Current item in connectlist for for loops

	/* First put together fd_set for select(), which will
	   consist of the sock veriable in case a new connection
	   is coming in, plus all the sockets we have already
	   accepted. */
	
	FD_ZERO(&socks);
	
	/* Loops through all the possible connections and adds
		those sockets to the fd_set */

	// Note that one thread handles only 2 sockets, one from the
	// client, the other to the VM!
	
	for (listnum = 0; listnum < 2; listnum++) {
		if (connectlist[listnum] != 0) {
			FD_SET(connectlist[listnum],&socks);
			if (connectlist[listnum] > highsock) {
				highsock = connectlist[listnum];
			}
		}
	}
}

// Manage the actual forwarding of data
void *deal_with_data(void *sockptr) {

	struct arg_thread arg = *(struct arg_thread *)sockptr;

	int client_socket = arg.socket;
	int	vm_socket = get_current_socket(arg.ip_address);
	
	pthread_mutex_unlock(&mutex);
	
	int times; // Number of reallocation
	times = 2;

	char *cur_char;
	int readsocks; // Number of sockets ready for reading
	int my_service;

	struct timeval timeout;

	int bytes_ready_from_client = 0;
	int bytes_ready_to_client = 0;

	int transferred_bytes; // How many bytes are transferred by a sock_write or sock_read operation

	buffer_from_client = malloc(FORWARD_BUFFER_SIZE);
	buffer_to_client = malloc(FORWARD_BUFFER_SIZE);
	aux_buffer_from_client = malloc(FORWARD_BUFFER_SIZE);
	aux_buffer_to_client = malloc(FORWARD_BUFFER_SIZE);

	bzero(buffer_from_client, FORWARD_BUFFER_SIZE);
	bzero(buffer_to_client, FORWARD_BUFFER_SIZE);
	bzero(aux_buffer_from_client, FORWARD_BUFFER_SIZE);
	bzero(aux_buffer_to_client, FORWARD_BUFFER_SIZE);

	bzero(&connectlist, sizeof(connectlist));

	connectlist[0] = client_socket;	
	connectlist[1] = vm_socket;

	// We never finish forwarding data!
	while (1) {
		
                build_select_list();

				// Setup a timeout
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                readsocks = select(highsock + 1, &socks, &socks, (fd_set *) 0, &timeout);

                if (readsocks < 0) {
                        perror("select");
                        exit(EXIT_FAILURE);
                }
                if (readsocks == 0) {
                        printf("(%d)", tid);
                        fflush(stdout);
                } else {
					
			// Check if the client is ready
			if(FD_ISSET(client_socket, &socks)) {

				// First of all, check if we have something for the client
				if(bytes_ready_to_client > 0) {
					//printf("Writing to client on socket %d:\n%s\n---\n",client_socket, (char *)buffer_to_client);
					printf("Write on client %s on sock %d\n", arg.ip_address, client_socket);
					if((transferred_bytes = sock_write(client_socket, buffer_to_client, bytes_ready_to_client)) < 0) {
						perror("write: sending to client from buffer");
					}

					bytes_ready_to_client = 0;
					bzero(buffer_to_client, FORWARD_BUFFER_SIZE);
				}

	            // Always perform sock_read, if it returns a number greater than zero
	            // something has been read then we've to append aux buffer to previous buffer (pointers)
	            transferred_bytes = sock_read(client_socket, aux_buffer_from_client, FORWARD_BUFFER_SIZE);
	            if(transferred_bytes < 0 && transferred_bytes != NOT_AVAILABLE) {
					perror("read: reading from client");
	            }
	                
	            if(transferred_bytes == 0){
					free(buffer_from_client);
					free(buffer_to_client);
					free(aux_buffer_from_client);
					free(aux_buffer_to_client);
					close(vm_socket);
					close(client_socket);
					pthread_exit(NULL);
				}
	                
	            if(transferred_bytes > 0){
					append_buffer(buffer_from_client,aux_buffer_from_client,&bytes_ready_from_client,transferred_bytes,&times);	
				}
				
				if(bytes_ready_from_client > 0) {
					//printf("Read from client on socket %d:\n%s\n---\n",client_socket, (char *)buffer_from_client);
					printf("Read from client %s on socket %d\n", arg.ip_address, client_socket);
				}
				
			}
			// Check if the VM is ready
			if(FD_ISSET(vm_socket, &socks)) {

				// First of all, check if we have something for the VM to send
				if(bytes_ready_from_client > 0) {

					//printf("Writing to VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_from_client);
					printf("Write to VM on socket %d\n", vm_socket);
                    if((transferred_bytes = sock_write(vm_socket, buffer_from_client, bytes_ready_from_client)) < 0) {
						perror("write: sending to vm from client");
                    }

                    bytes_ready_from_client = 0;
					bzero(buffer_from_client, FORWARD_BUFFER_SIZE);
                }

	            // Always perform sock_read, if it returns a number greater than zero
	            // something has been read then we've to append aux buffer to previous buffer (pointers)
	            transferred_bytes = sock_read(vm_socket, aux_buffer_to_client, FORWARD_BUFFER_SIZE);
	            if(transferred_bytes < 0 && transferred_bytes != -71) {
					perror("read: reading from vm");
	            }
	       
	            if(transferred_bytes == 0){
					free(buffer_from_client);
					free(buffer_to_client);
					free(aux_buffer_from_client);
					free(aux_buffer_to_client);
					close(vm_socket);
					close(client_socket);
					pthread_exit(NULL);
				}
	                
	            if(transferred_bytes > 0){
					append_buffer(buffer_to_client,aux_buffer_to_client,&bytes_ready_to_client,transferred_bytes,&times);
				}
				
				if(bytes_ready_to_client > 0) {
					//printf("Read from VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_to_client);
					printf("Read from VM on socket %d\n", vm_socket);
				}

			}

                }
        }

}


void create_empty_list(struct vm_data * tpcw_instance){
	int index;
	for(index = 0; index < MAX_CONNECTED_CLIENTS; index++){
		tpcw_instance->connected_clients[index] = (char *)malloc(16);
		strcpy(tpcw_instance->connected_clients[index],"0.0.0.0");
		//printf("tpcw_instance->connected_cliets[%d]: %s\n", index, tpcw_instance->connected_clients[index]);
	}
}


/*
 * In this function we manage the vms in the system
 * 
 * ADD: a new vm has to be added to the system
 * DELETE: an existent vm has to be deleted from the system (we have to manage client connection)
 * REJUVENTATING: an existent vm has to perform rejuvenation (we have to manage client connection)
 */
void * controller_thread(void * v){
	
	printf("Controller thread up!\n");
	int socket;
	socket = (int)(long)v;
	// struct virtual_machine represents all the needed info to perform a connection with it
	struct virtual_machine{
		char ip[16];			// vm's ip address
		int port;				// vm's port number
		enum services service;	// service provided by the vm
		enum operations op;		// operation performed by the controller
	};
	
	struct virtual_machine vm;
	
	printf("Waiting for communication by the controller...\n");
	while(1){
		// Wait for info by the controller
		if ((recv(socket, &vm, sizeof(struct virtual_machine),0)) == -1){
				perror("controller_thread - recv");
				if (errno == EWOULDBLOCK || errno == EAGAIN) {
					perror("controller_thread - recv timeout");
					fflush(stdout);
				}
				close(socket);
		}
		printf("New info received by the controller\n");
		printf("Received VM with IP: %s, PORT: %d, SERVICE: %d, OP: %d\n", vm.ip, vm.port,vm.service, vm.op);
	
		// This function performs a realloc if the size is not enough to guest a new VM
		// TODO: sistemare perchè così potrei fare un controllo in più se operazione non è ADD
		check_vm_data_set_size(vm.service);
	
		// It is a pointer to a vm_data struct in the vm_data_set
		struct vm_data * temp_vm_data;
		temp_vm_data = &vm_data_set[vm.service][current_vms[vm.service]];
		
		create_empty_list(temp_vm_data);
		
		// CNT contacts LB just for active CN
		// check operation field
		if(vm.op == ADD){
			strcpy(temp_vm_data->ip_address, vm.ip);
			temp_vm_data->port = vm.port;
			current_vms[vm.service]++;
		}
		else if(vm.op == DELETE){
			delete_vm(vm.ip, vm.service, vm.port);
		}
		else if(vm.op == REJ){
			delete_vm(vm.ip, vm.service, vm.port);
		}
		else{
			// something wrong, operation not supported!
			printf("In controller_thread: operation not supported!\n");
		}
	}
}

/*
 * This function allocates the memory to guest the system representation
 */
void create_system_image(){
	int index;
	for(index = 0; index < NUMBER_GROUPS; index++){
		current_vms[index] = 0;
		actual_index[index] = 0;
		// we need the second control because malloc may return NULL even if its argument is zero
		if((vm_data_set[index] = malloc(sizeof(struct vm_data)*NUMBER_VMs)) == NULL && (NUMBER_VMs != 0))
			exit(EXIT_FAILURE);
		allocated_vms[index] = NUMBER_VMs;
	}
}


int main (int argc, char *argv[]) {
	char *ascport;						//Service name
	short int port;       				//Port related to the service name
	struct sockaddr_in server_address;	//Forwarder reachability infos
	struct sockaddr_in controller; 		//Used to connect to controller
	int reuse_addr = 1;					//Flag to set socket properties
	struct timeval timeout;				
	int readsocks; 						//Number of sockets ready for reading
	int connection;						//Client socket number
	int sockfd_controller;				//Socket number for controller connection
	
	pthread_attr_t pthread_custom_attr;
	pthread_t tid;

	// Service_name: used to collect all the info about the forwarder net infos
	// ip_controller: ip used to contact the controller
	// port_controller: port number used to contact the controller
	if (argc != 5) {
		printf("Usage: %s Service_name ip_controller port_controller port_tpcw\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/* CONNECTION LB - CONTROLLER */
	printf("Creating socket to controller...\n");
	sockfd_controller = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd_controller < 0) {
		perror("main: socket_controller");
		exit(EXIT_FAILURE);
	}
	// Controller's info
	controller.sin_family = AF_INET;
	controller.sin_addr.s_addr = inet_addr(argv[2]);
	controller.sin_port = htons(atoi(argv[3]));
	
	// Allocates memory for representing the system
	create_system_image();
	
	// Connect to controller (it creates the connection LB - Controller)
	if (connect(sockfd_controller, (struct sockaddr *)&controller , sizeof(controller)) < 0) {
        perror("main: connect_to_controller");
        exit(EXIT_FAILURE);
    }
    printf("Correctely connected to controller %s\n", inet_ntoa(controller.sin_addr));

	// Once connection is created, build up a new thread to implement the exchange of messages between LB and Controller
	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&tid,&pthread_custom_attr,controller_thread,(void *)(long)sockfd_controller);

	/* CONNECTION LB - CLIENTS */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Avoid TIME_WAIT and set the socket as non blocking
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

	// Get the address information, and bind it to the socket
	ascport = argv[1]; //argv[1] has to be a service name (not a port)
	//port = atoport(ascport, NULL); //sockethelp.c
	port = atoi(argv[4]);
	memset((char *) &server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port =htons(port);
	if (bind(sock, (struct sockaddr *) &server_address,
	  sizeof(server_address)) < 0 ) {
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	if(listen(sock, MAX_NUM_OF_CLIENTS) < 0){
		perror("listen: ");
	}
	printf("Listening on port %d\n", port);
	
	// Accepting clients
	while(1) {
		
		struct sockaddr_in client;
		unsigned int addr_len;
		addr_len = sizeof(struct sockaddr_in);
		connection = accept(sock, (struct sockaddr *)&client, &addr_len);
	        if (connection < 0) {
	                perror("accept");
	                exit(EXIT_FAILURE);
        	}
        	
		setnonblocking(connection);

		printf("accepted connection on sockid %d from client %s\n", connection, inet_ntoa(client.sin_addr));

		struct arg_thread vm_client;
		
		pthread_mutex_lock(&mutex);
		vm_client.socket = connection;
		strcpy(vm_client.ip_address,inet_ntoa(client.sin_addr));
		vm_client.port = ntohs(client.sin_port);
		
		printf("New client connected from <%s, %d>\n", vm_client.ip_address, vm_client.port);
		res_thread = create_thread(deal_with_data, &vm_client);
		if(res_thread != 0){
			pthread_mutex_unlock(&mutex);
		}

	}
}
