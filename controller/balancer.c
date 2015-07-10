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
#include "timer.h"
#include <stdlib.h>
#include "vm_list.h"

#define MAX_NUM_OF_CLIENTS	 	1024			//Max number of accepted clients
#define FORWARD_BUFFER_SIZE		1024*1024		//Size of buffers
#define NUMBER_VMs				1024				//It must be equal to the value in server side in controller
#define NUMBER_GROUPS			3				//It must be equal to the value in server side in controller
#define MAX_CONNECTED_CLIENTS	1024				//It represents the max number of connected clients
#define NOT_AVAILABLE			-71
#define UPDATE_LOCAL_REGION_FEATURE_INTERVAL	10				//interval in seconds
#define NUMBER_REGIONS			4
#define VM_SERVICE_PORT			8080

pthread_mutex_t mutex;
int res_thread;
int lambda = 0;
timer update_local_region_features_timer;
char my_own_ip[16];
int port_remote_balancer;
int socket_remote_balancer;

struct vm_list_elem *vm_list;

struct _region regions[NUMBER_REGIONS];

//Used to pass client info to threads
struct arg_thread {
	int socket;
	char ip_address[16];
	int port;
	int user_type;
};

void setnonblocking(int sock);

/*
 * check for the vm_data_set size
 */

// Append to the original buffer the content of aux_buffer
void append_buffer(char * original_buffer, char * aux_buffer,
		int * bytes_original, int bytes_aux, int * times) {

	if ((*bytes_original + bytes_aux) >= FORWARD_BUFFER_SIZE) {
		printf("REALLOC: STAMPA TIMES: %d\n", *times);
		original_buffer = realloc(original_buffer,
				(*times) * FORWARD_BUFFER_SIZE);
		printf("HO FATTO LA REALLOC!\n");
		(*times)++;
	}
	//strncpy(&original_buffer[*bytes_original],aux_buffer,bytes_aux);
	//strncpy(&(original_buffer[*bytes_original]),aux_buffer,(FORWARD_BUFFER_SIZE - *bytes_original));
	memcpy(&(original_buffer[*bytes_original]), aux_buffer, bytes_aux);
	*bytes_original = *bytes_original + bytes_aux;
	bzero(aux_buffer, FORWARD_BUFFER_SIZE);
}

struct sockaddr_in select_local_vm_addr() {
	//print_vm_list(vm_list);
	static int current_rr_index = 0;
	struct sockaddr_in target_vm_saddr;
	target_vm_saddr.sin_family = AF_INET;
	//pthread_mutex_lock(&mutex);
	if (vm_list_size(vm_list) == 0)
		return target_vm_saddr;
	if (current_rr_index >= vm_list_size(vm_list))
		current_rr_index = 0;
	struct virtual_machine *vm = get_vm_by_position(current_rr_index, vm_list);
	current_rr_index++;
	target_vm_saddr.sin_addr.s_addr = inet_addr(vm->ip);
	target_vm_saddr.sin_port = htons(VM_SERVICE_PORT);
	//printf("Current vm list size: %i, current index %i,  selected vm %s\n", vm_list_size(), current_rr_index, vm->ip);
	//pthread_mutex_unlock(&mutex);
	return target_vm_saddr;
}

// select the server address
struct sockaddr_in get_target_server_saddr(char * ip, int port, int user_type) {
	struct sockaddr_in target_server_saddr;
	target_server_saddr.sin_family = AF_INET;
	int index;
	float probability_sum;
	float random;

	switch (user_type) {

	case 0: //from a user
		probability_sum = 0;
		random = (float) rand() / (float) RAND_MAX;
		index = 0;
		probability_sum = regions[index].probability;
		while (random > probability_sum && index < NUMBER_REGIONS) {
			index++;
			probability_sum += regions[index].probability;
		}
		if (!strcmp(regions[index].ip_balancer,	my_own_ip) || index == NUMBER_REGIONS) {
			target_server_saddr = select_local_vm_addr();
			printf("New user <%s, %d> forwarded to local server %s\n", ip, port, inet_ntoa(target_server_saddr.sin_addr));
			return target_server_saddr;
		} else {
			target_server_saddr.sin_addr.s_addr = inet_addr(regions[index].ip_balancer);
			target_server_saddr.sin_port = htons(port_remote_balancer);
			printf("New user <%s, %d> forwarded to remote balancer %s\n", ip,
					port, regions[index].ip_balancer);
			return target_server_saddr;
		}

	case 1: //from a remote balancer
		target_server_saddr = select_local_vm_addr();
		printf(
				"Request from remote balancer <%s, %d> forwarded to local server %s\n",
				ip, port, inet_ntoa(target_server_saddr.sin_addr));
		return target_server_saddr;
	}
}

int create_socket(char * ip_client, int port_client, int user_type) {

	int sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id < 0) {
		perror("Error while creating socket for new client");
		return 0;
	}
	struct sockaddr_in saddr = get_target_server_saddr(ip_client, port_client,
			user_type);
	//printf("Connecting socket to server: %s\n", inet_ntoa(saddr.sin_addr));
	if (connect(sock_id, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		perror("Error while connecting socket for new client");
		return 0;
	}
	setnonblocking(sock_id);
	return sock_id;
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

void *update_region_features(void * sock) {
	int sockfd;
	sockfd = (int) (long) sock;
	int index;
	/*
	 struct sockaddr_in controller;
	 unsigned int addr_len;
	 addr_len = sizeof(struct sockaddr_in);*/
	struct _region temp_regions[NUMBER_REGIONS];
	while (1) {

		double time = timer_value_seconds(update_local_region_features_timer);
		float local_region_user_request_arrival_rate = (float) lambda
				/ (float) time;
		if (sock_write(sockfd, &local_region_user_request_arrival_rate,
				sizeof(float)) < 0)
			perror("Error in writing local arrival rate to controller");
		memset(temp_regions, 0, sizeof(struct _region) * NUMBER_REGIONS);
		if (sock_read(sockfd, &temp_regions,
				sizeof(struct _region) * NUMBER_REGIONS) < 0) {
			perror("Error in reading probabilities from the leader");
		}
		//pthread_mutex_lock(&mutex);
		memcpy(&regions, &temp_regions,
				sizeof(struct _region) * NUMBER_REGIONS);
		//pthread_mutex_unlock(&mutex);
		printf("-----------------\nRegion distribution probabilities:\n");
		for (index = 0; index < NUMBER_REGIONS; index++) {
			if (strnlen(regions[index].ip_controller, 16) != 0) {
				printf("Balancer %s\t %f\n", regions[index].ip_balancer,
						regions[index].probability);
			}
		}
		printf("-----------------\n");
		timer_restart(update_local_region_features_timer);
		printf("LAMBDA IS: %d and INTERVAL IS: %d\n", lambda,
				UPDATE_LOCAL_REGION_FEATURE_INTERVAL);
		printf("Sent arrival rate is %.3f. Timer restarted!\n",
				local_region_user_request_arrival_rate);
		lambda = 0;
		while (timer_value_seconds(update_local_region_features_timer)
				< UPDATE_LOCAL_REGION_FEATURE_INTERVAL) {
			sleep(1);
		}
	}
}

// Manage the actual forwarding of data
void *client_sock_id_thread(void *vm_client_arg) {

	void *buffer_from_client;
	void *buffer_to_client;
	void *aux_buffer_from_client;
	void *aux_buffer_to_client;
	int connectlist[2];  // One thread handles only 2 sockets
	fd_set socks; // Socket file descriptors we want to wake up for, using select()
	int highsock;
	struct arg_thread vm_client = *(struct arg_thread *) vm_client_arg;

	int client_socket = vm_client.socket;
	//printf("Creating socket for new user...\n");
	int vm_socket = create_socket(vm_client.ip_address, vm_client.port,
			vm_client.user_type);

	if (vm_socket==0) {
		free(buffer_from_client);
		free(buffer_to_client);
		free(aux_buffer_from_client);
		free(aux_buffer_to_client);
		close(vm_socket);
		close(client_socket);
		free(vm_client_arg);
		pthread_exit(NULL);
	}

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
	bzero(aux_buffer_from_client, FORWARD_BUFFER_SIZE);
	bzero(aux_buffer_to_client, FORWARD_BUFFER_SIZE);

	bzero(&connectlist, sizeof(connectlist));

	connectlist[0] = client_socket;
	connectlist[1] = vm_socket;

	// We never finish forwarding data!
	while (1) {

		int listnum; //Current item in connectlist for for loops
		/* First put together fd_set for select(), which will
		 consist of the sock veriable in case a new client_sock_id
		 is coming in, plus all the sockets we have already
		 accepted. */
		FD_ZERO(&socks);
		/* Loops through all the possible client_sock_ids and adds those sockets to the fd_set */
		// Note that one thread handles only 2 sockets, one from the  client, the other to the VM!
		for (listnum = 0; listnum < 2; listnum++) {
			if (connectlist[listnum] != 0) {
				FD_SET(connectlist[listnum], &socks);
				if (connectlist[listnum] > highsock) {
					highsock = connectlist[listnum];
				}
			}
		}

		// Setup a timeout
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		readsocks = select(highsock + 1, &socks, &socks, (fd_set *) 0,
				&timeout);

		if (readsocks < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		if (readsocks == 0) {
			printf("(%d)", tid);
			fflush(stdout);
		} else {

			// Check if the client is ready
			if (FD_ISSET(client_socket, &socks)) {
				// First of all, check if we have something for the client
				if (bytes_ready_to_client > 0) {
					//printf("Writing to client on socket %d:\n%s\n---\n",client_socket, (char *)buffer_to_client);
					//printf("Writing to client on socket: %d on actual_index: %d\n", client_socket, actual_index[0]);
					if ((transferred_bytes = sock_write(client_socket,
							buffer_to_client, bytes_ready_to_client)) < 0) {
						perror("write: sending to client from buffer");
					}

					bytes_ready_to_client = 0;
					bzero(buffer_to_client, FORWARD_BUFFER_SIZE);
				}

				// Always perform sock_read, if it returns a number greater than zero
				// something has been read then we've to append aux buffer to previous buffer (pointers)
				transferred_bytes = sock_read(client_socket,
						aux_buffer_from_client, FORWARD_BUFFER_SIZE);
				if (transferred_bytes < 0 && transferred_bytes != NOT_AVAILABLE) {
					perror("read: reading from client");
				}

				if (transferred_bytes == 0) {
					free(buffer_from_client);
					free(buffer_to_client);
					free(aux_buffer_from_client);
					free(aux_buffer_to_client);
					close(vm_socket);
					close(client_socket);
					free(vm_client_arg);
					pthread_exit(NULL);
				}

				if (transferred_bytes > 0) {
					append_buffer(buffer_from_client, aux_buffer_from_client,
							&bytes_ready_from_client, transferred_bytes,
							&times);
				}

				if (bytes_ready_from_client > 0) {
					//printf("Read from client on socket %d:\n%s\n---\n",client_socket, (char *)buffer_from_client);
					//printf("Read from client on socket: %d on actual_index: %d\n", client_socket, actual_index[0]);
					if (!vm_client.user_type)
						lambda++;
				}

			}
			// Check if the VM is ready
			if (FD_ISSET(vm_socket, &socks)) {
				// First of all, check if we have something for the VM to send
				if (bytes_ready_from_client > 0) {
					//printf("Writing to VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_from_client);
					//printf("Writing to VM on socket: %d on actual_index: %d\n", vm_socket, actual_index[0]);

					if ((transferred_bytes = sock_write(vm_socket,
							buffer_from_client, bytes_ready_from_client)) < 0) {
						perror("write: sending to vm from client");
					}
					bytes_ready_from_client = 0;
					bzero(buffer_from_client, FORWARD_BUFFER_SIZE);
				}

				// Always perform sock_read, if it returns a number greater than zero
				// something has been read then we've to append aux buffer to previous buffer (pointers)
				transferred_bytes = sock_read(vm_socket, aux_buffer_to_client,
						FORWARD_BUFFER_SIZE);
				if (transferred_bytes < 0 && transferred_bytes != -71) {
					perror("read: reading from vm");
				}

				if (transferred_bytes == 0) {
					free(buffer_from_client);
					free(buffer_to_client);
					free(aux_buffer_from_client);
					free(aux_buffer_to_client);
					close(vm_socket);
					close(client_socket);
					free(vm_client_arg);
					pthread_exit(NULL);
				}

				if (transferred_bytes > 0) {
					append_buffer(buffer_to_client, aux_buffer_to_client,
							&bytes_ready_to_client, transferred_bytes, &times);
				}

				if (bytes_ready_to_client > 0) {
					//printf("Read from VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_to_client);
					//printf("Read from VM on socket: %d on actual_index: %d\n", vm_socket, actual_index[0]);
				}

			}

		}
	}

}

/*
 * In this function we manage the vms in the system
 * 
 * ADD: a new vm has to be added to the system
 * DELETE: an existent vm has to be deleted from the system (we have to manage client client_sock_id)
 * REJUVENTATING: an existent vm has to perform rejuvenation (we have to manage client client_sock_id)
 */
void * controller_thread(void * v) {
	printf("Controller thread is running\n");
	int socket;
	int numbytes;
	socket = (int) (long) v;
	struct virtual_machine_operation vm_op;
	printf("Waiting commands from controller...\n");
	while (1) {
		// Wait for info by the controller

		if ((numbytes = sock_read(socket,&vm_op,sizeof(struct virtual_machine_operation))) == -1) {
            printf("Failed receiving data from controller\n");
            perror("sock_read: ");
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout on sock_read() while waiting data from controller\n");
            }
            close(socket);
            break;
        } else  if (numbytes == 0) {
            printf("Controller is disconnected\n");
            close(socket);
            break;
        }


		printf("Operation %i received by controller for vm %s\n", vm_op.op,	vm_op.ip);
		if (vm_op.op == ADD) {
			struct virtual_machine * vm = (struct virtual_machine*) malloc(sizeof(struct virtual_machine));
			memcpy(vm->ip, vm_op.ip, 16);
			printf("Adding vm %s\n", vm->ip);
			pthread_mutex_lock(&mutex);
			add_vm(vm, &vm_list);
			printf("-----------------\nNew vm list:\n");
			print_vm_list(vm_list);
			pthread_mutex_unlock(&mutex);
		} else if (vm_op.op == DELETE) {
			printf("Removing vm %s\n", vm_op.ip);
			pthread_mutex_lock(&mutex);
			remove_vm_by_ip(vm_op.ip, &vm_list);
			printf("-----------------\nNew vm list:\n");
			print_vm_list(vm_list);
			pthread_mutex_unlock(&mutex);
		} else if (vm_op.op == REJ) {
			printf("Removing vm %s\n", vm_op.ip);
			pthread_mutex_lock(&mutex);
			remove_vm_by_ip(vm_op.ip, &vm_list);
			printf("-----------------\nNew vm list:\n");
			print_vm_list(vm_list);
			pthread_mutex_unlock(&mutex);
		} else {
			// something wrong, operation not supported!
			printf("Received not supported operation from controller\n");
		}
	}
}

void get_my_own_ip() {
	/* LOCAL */
	/*
	 struct ifaddrs *ifaddr, *ifa;
	 int family;
	 getifaddrs(&ifaddr);
	 for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
	 if (ifa->ifa_addr == NULL)
	 continue;

	 family = ifa->ifa_addr->sa_family;

	 if (family == AF_INET && !strcmp(ifa->ifa_name,"eth0")) {
	 strcpy(my_own_ip,inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr));
	 goto ip_found;
	 }
	 }
	 printf("Unable to get my own ip!\n");
	 ip_found:
	 freeifaddrs(ifaddr);*/

	/* AMAZON */

	FILE *f;
	f = popen("curl http://169.254.169.254/latest/meta-data/public-ipv4", "r");
	if (f == NULL)
		abort();

	//fgets(my_own_ip, 16, f);
	fscanf(f, "%s", my_own_ip);
	pclose(f);
}

void * accept_balancers(void * v) {
	while (1) {

		int client_sock_id;
		struct sockaddr_in client;
		unsigned int addr_len;
		addr_len = sizeof(struct sockaddr_in);
		client_sock_id = accept(socket_remote_balancer,
				(struct sockaddr *) &client, &addr_len);
		if (client_sock_id < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		setnonblocking(client_sock_id);

		struct arg_thread *vm_client = (struct arg_thread*) malloc(
				sizeof(struct arg_thread));

		vm_client->socket = client_sock_id;
		strcpy(vm_client->ip_address, inet_ntoa(client.sin_addr));
		vm_client->port = ntohs(client.sin_port);
		vm_client->user_type = 1;

		//printf("New balancer connected from <%s, %d>\n", vm_client->ip_address, vm_client->port);
		res_thread = create_thread(client_sock_id_thread, vm_client);

	}
}

int main(int argc, char *argv[]) {
	char *ascport;						//Service name
	short int port;       				//Port related to the service name
	struct sockaddr_in server_address;	//Forwarder reachability infos
	struct sockaddr_in controller; 		//Used to connect to controller
	int reuse_addr = 1;					//Flag to set socket properties
	struct timeval timeout;
	int readsocks; 						//Number of sockets ready for reading
	int client_sock_id;						//Client socket number
	int sock_id_controller;		//Socket number for controller client_sock_id
	int sock_id_update_region_features;
	int port_update_region_features;

	pthread_attr_t pthread_custom_attr;
	pthread_t tid;
	pthread_t tid_update_region_features;
	pthread_t tid_balancer;

	// Service_name: used to collect all the info about the forwarder net infos
	// ip_controller: ip used to contact the controller
	// port_controller: port number used to contact the controller
	if (argc != 7) {
		printf(
				"Usage: %s Service_name ip_controller port_controller port_to_update_region_features port_tpcw port_remote_lb\n",
				argv[0]);
		exit(EXIT_FAILURE);
	}

	vm_list = NULL;
	port_update_region_features = atoi(argv[4]);
	port_remote_balancer = atoi(argv[6]);
	/* CONNECTION LB - CONTROLLER */
	printf("Creating socket to controller...\n");
	sock_id_controller = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id_controller < 0) {
		perror("main: socket_controller");
		exit(EXIT_FAILURE);
	}
	// Controller's info
	controller.sin_family = AF_INET;
	controller.sin_addr.s_addr = inet_addr(argv[2]);
	controller.sin_port = htons(atoi(argv[3]));

	// Allocates memory for representing the system
	if (connect(sock_id_controller, (struct sockaddr *) &controller,
			sizeof(controller)) < 0) {
		perror("main: connect_to_controller");
		exit(EXIT_FAILURE);
	}
	// Send to controller balancer public ip address
	get_my_own_ip();
	if (sock_write(sock_id_controller, my_own_ip, 16) < 0) {
		perror(
				"Error in sending balancer public ip address to its own controller: ");
	}
	printf(
			"Balancer %s correctely connected to its own controller %s on port %d\n",
			my_own_ip, inet_ntoa(controller.sin_addr),
			ntohs(controller.sin_port));

	// Once client_sock_id is created, build up a new thread to implement the exchange of messages between LB and Controller
	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&tid, &pthread_custom_attr, controller_thread,
			(void *) (long) sock_id_controller);

	/* CONNECTION LB - CONTROLLER ARRIVAL RATE */
	printf("Creating socket to controller arrival rate...\n");
	sock_id_update_region_features = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id_update_region_features < 0) {
		perror("main: socket_controller_arrival_rate");
		exit(EXIT_FAILURE);
	}
	// Controller's info
	controller.sin_family = AF_INET;
	controller.sin_addr.s_addr = inet_addr(argv[2]);
	controller.sin_port = htons(port_update_region_features);

	// Connect to controller (it if (bind(sock, (struct sockaddr *) &server_address,
	if (connect(sock_id_update_region_features, (struct sockaddr *) &controller,
			sizeof(controller)) < 0) {
		perror("main: connect_to_controller arrival rate");
		exit(EXIT_FAILURE);
	}
	printf("Correctly connected to controller %s on port %d\n",
			inet_ntoa(controller.sin_addr), port_update_region_features);

	memset(regions, 0, sizeof(struct _region) * NUMBER_REGIONS);
	// Once client_sock_id is created, build up a new thread to implement the exchange of messages between LB and Controller
	pthread_attr_init(&pthread_custom_attr);
	timer_start(update_local_region_features_timer);
	pthread_create(&tid_update_region_features, &pthread_custom_attr,
			update_region_features,
			(void *) (long) sock_id_update_region_features);

	/* CONNECTION LB - CLIENTS */
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Avoid TIME_WAIT and set the socket as non blocking
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

	// Get the address information, and bind it to the socket
	ascport = argv[1]; //argv[1] has to be a service name (not a port)
	port = atoi(argv[5]); //sockethelp.c
	memset((char *) &server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address))
			< 0) {
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}

	if (listen(sock, MAX_NUM_OF_CLIENTS) < 0) {
		perror("listen: ");
	}
	printf("Listening on port %d\n", port);

	struct sockaddr_in balancer_address;

	balancer_address.sin_family = AF_INET;
	balancer_address.sin_addr.s_addr = htonl(INADDR_ANY);
	balancer_address.sin_port = htons(port_remote_balancer);

	socket_remote_balancer = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_remote_balancer < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	if (bind(socket_remote_balancer, (struct sockaddr *) &balancer_address,
			sizeof(balancer_address)) < 0) {
		perror("bind");
		close(socket_remote_balancer);
		exit(EXIT_FAILURE);
	}

	if (listen(socket_remote_balancer, MAX_NUM_OF_CLIENTS) < 0) {
		perror("listen: ");
	}
	if (listen(socket_remote_balancer, MAX_NUM_OF_CLIENTS) < 0) {
		perror("listen: ");
	}
	printf(
			"Listening on port for incoming client_sock_id from other balancers %d\n",
			port_remote_balancer);
	pthread_create(&tid_balancer, &pthread_custom_attr, accept_balancers, NULL);

	// Accepting clients
	while (1) {

		struct sockaddr_in client;
		unsigned int addr_len;
		addr_len = sizeof(struct sockaddr_in);
		client_sock_id = accept(sock, (struct sockaddr *) &client, &addr_len);
		//printf("New client connected from %s\n", inet_ntoa(client.sin_addr));
		if (client_sock_id < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		//printf("Setting socket non blocking for cliet %s\n", inet_ntoa(client.sin_addr));
		setnonblocking(client_sock_id);
		//printf("Set socket non blocking for cliet %s\n", inet_ntoa(client.sin_addr));
		if (vm_list_size(vm_list) == 0) {
			printf("Vm list size is equal to zero. Rejecting client %s\n",
					inet_ntoa(client.sin_addr));
			close(client_sock_id);
			continue;
		}

		//printf("Accepted for cliet %s\n", inet_ntoa(client.sin_addr));
		struct arg_thread *vm_client = (struct arg_thread*) malloc(
				sizeof(struct arg_thread));
		vm_client->socket = client_sock_id;
		strcpy(vm_client->ip_address, inet_ntoa(client.sin_addr));
		vm_client->port = ntohs(client.sin_port);
		vm_client->user_type = 0;

		//printf("Creating new thread for client <%s, %d>\n", vm_client->ip_address, vm_client->port);
		//fflush(stdout);
		res_thread = create_thread(client_sock_id_thread, vm_client);

	}
}

