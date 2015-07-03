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

#define MAX_NUM_OF_CLIENTS		1024			//Max number of accepted clients
#define FORWARD_BUFFER_SIZE		1024*1024		//Size of buffers
#define NUMBER_VMs				1024				//It must be equal to the value in server side in controller
#define NUMBER_GROUPS			3				//It must be equal to the value in server side in controller
#define MAX_CONNECTED_CLIENTS		1024				//It represents the max number of connected clients
#define NOT_AVAILABLE			-71
#define ARRIVAL_RATE_INTERVAL	10				//interval in seconds
#define NUMBER_REGIONS			4

int current_vms[NUMBER_GROUPS];				//Number of connected VMs
int allocated_vms[NUMBER_GROUPS];			//Number of possible VMs
int index_vms[NUMBER_GROUPS];				//It represents (for each service) the VM that has to be contacted
int actual_index[NUMBER_GROUPS];			//It represents the actual index to assign VM
pthread_mutex_t mutex;
int res_thread;
int lambda = 0;
float arrival_rate = 0.0;
timer arrival_rate_timer;
char my_own_ip[16];
int port_remote_balancer;
int socket_remote_balancer;

// TODO: posso non discriminare DEL e REJ operations? Credo di si!
enum operations{
        ADD, DELETE, REJ
};


enum services{
        SERVICE_1, SERVICE_2, SERVICE_3
};

struct virtual_machine{
	char ip[16];                    // vm's ip address
	int port;                               // vm's port number
	enum services service;  // service provided by the vm
	enum operations op;             // operation performed by the controller
};

struct _vm_list_elem {
	struct virtual_machine vm;
	struct _vm_list_elem *next;
} vm_list_elem;

struct vm_list_elem *vm_list;


struct _region_features{
        float arrival_rate;
        float mttf;
};

struct _region{
        char ip_controller[16];
        char ip_balancer[16];
        struct _region_features region_features;
        float probability;
};

struct _region regions[NUMBER_REGIONS];

//Used to pass client info to threads
struct arg_thread{
	int socket;
	char ip_address[16];
	int port;
	int user_type;
};



//This struct is different from the struct in the controller, because balancer needs less infos than controller
struct vm_data{
	char ip_address[16];
	int port;
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
	
	pthread_mutex_lock(&mutex);
	for(index = 0; index < current_vms[service]; index++){
		if(strcmp(vm_data_set[service][index].ip_address,ip) == 0 && vm_data_set[service][index].port == port){
			if(current_vms[service] == 1){
				printf("No more TPCW instances are available\n");
				current_vms[service] = 0;
				break;
			}
			if(index == current_vms[service] -1){
				printf("The last active TPCW instance is %s\n", vm_data_set[service][index - 1].ip_address);
				current_vms[service]--;
				break;
			}
			printf("Deleted TPCW with ip %s and port %d\n", ip, port);
			printf("Deleting ip %s - Last ip %s\n", vm_data_set[service][index].ip_address, vm_data_set[service][current_vms[service]-1].ip_address);
			vm_data_set[service][index] = vm_data_set[service][--current_vms[service]];
			printf("Connected TPCW has ip %s and port %d\n", vm_data_set[service][index].ip_address, vm_data_set[service][index].port);
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
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


struct sockaddr_in select_local_server_saddr(){
        struct sockaddr_in target_server_saddr;
        target_server_saddr.sin_family = AF_INET;
        target_server_saddr.sin_addr.s_addr = inet_addr(vm_data_set[0][actual_index[0]].ip_address);
        target_server_saddr.sin_port = vm_data_set[0][actual_index[0]].port;
        if(actual_index[0] < (current_vms[0] - 1)){
                actual_index[0]++;
        }
        else actual_index[0] = 0;
        printf("Selected server <i%s, %d>\n", vm_data_set[0][actual_index[0]].ip_address, vm_data_set[0][actual_index[0]].port);
        return target_server_saddr;
}


// select the server address
struct sockaddr_in get_target_server_saddr(char * ip, int port, int user_type){
        struct sockaddr_in target_server_saddr;
        target_server_saddr.sin_family = AF_INET;
        int index;
        float probability_sum;
        float random;


        switch(user_type) {

                case 0: //from a user
                        probability_sum = 0;
                        random = (float)rand()/(float)RAND_MAX;
                        index = 0;
                        probability_sum = regions[index].probability;
                        while(random > probability_sum && index < NUMBER_REGIONS){
                                index++;
                                probability_sum += regions[index].probability;
                        }
                        if(!strcmp(regions[index].ip_balancer,my_own_ip) || index == NUMBER_REGIONS){
                                printf("New user <i%s, %d> forwarded to local region\n", ip, port);
                                return select_local_server_saddr();
                        } else{
                                target_server_saddr.sin_addr.s_addr = inet_addr(regions[index].ip_balancer);
                                target_server_saddr.sin_port = htons(port_remote_balancer);
                                printf("New user <%s, %d> forwarded to remote balancer %s\n", ip, port, regions[index].ip_balancer);
                                return target_server_saddr;
                        }

                case 1: //from a remote balancer
                        printf("Request from remote balancer <%s, %d> forwarded to local region\n", ip, port);
                        return select_local_server_saddr();
        }
}


int create_socket(char * ip_client, int port_client, int user_type) {

        int sock_id = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_id< 0) {
                perror("Error while creating socket for new user");
                exit(EXIT_FAILURE);
        }
        struct sockaddr_in saddr = get_target_server_saddr(ip_client,port_client,user_type);
        if (connect(sock_id, (struct sockaddr *)&saddr , sizeof(saddr)) < 0) {
                perror("Error while connecting socket for new user");
                exit(EXIT_FAILURE);
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

void *update_region_features(void * sock){
	int sockfd;
	sockfd = (int)(long)sock;
	int index;
	/*
	struct sockaddr_in controller;
	unsigned int addr_len;
	addr_len = sizeof(struct sockaddr_in);*/
	struct _region temp_regions[NUMBER_REGIONS];
	while(1){
		
		double time = timer_value_seconds(arrival_rate_timer);
		arrival_rate = (float)lambda/(float)time;
		if(sock_write(sockfd,&arrival_rate,sizeof(float)) < 0)
			perror("Error in writing arrival rate to controller: ");
		memset(temp_regions,0,sizeof(struct _region)*NUMBER_REGIONS);
		if(sock_read(sockfd,&temp_regions,sizeof(struct _region)*NUMBER_REGIONS) < 0 ){
			perror("Error in reading probabilities from the leader: ");
		}
		pthread_mutex_lock(&mutex);
		memcpy(&regions,&temp_regions,sizeof(struct _region)*NUMBER_REGIONS);
		pthread_mutex_unlock(&mutex);
		printf("-----------------\nRegion distribution probabilities:\n");
        	for(index = 0; index < NUMBER_REGIONS; index++){
                	if(strnlen(regions[index].ip_controller,16) != 0){
                       		printf("Balancer %s\t %f\n", regions[index].ip_balancer, regions[index].probability);
                	}
        	}
        	printf("-----------------\n");
		timer_restart(arrival_rate_timer);
		printf("LAMBDA IS: %d and INTERVAL IS: %d\n", lambda, ARRIVAL_RATE_INTERVAL);
		printf("Sent arrival rate is %.3f. Timer restarted!\n", arrival_rate);
			lambda = 0;
		while(timer_value_seconds(arrival_rate_timer) < ARRIVAL_RATE_INTERVAL){
			//printf("Timer value seconds: %f\n", timer_value_seconds(arrival_rate_timer));
			sleep(1);
		}
	}
}

// Manage the actual forwarding of data
void *connection_thread(void *vm_client_arg) {

      void *buffer_from_client;
        void *buffer_to_client;
        void *aux_buffer_from_client;
        void *aux_buffer_to_client;
        int connectlist[2];  // One thread handles only 2 sockets
        fd_set socks; // Socket file descriptors we want to wake up for, using select()
        int highsock;
        struct arg_thread vm_client = *(struct arg_thread *)vm_client_arg;

        int client_socket = vm_client.socket;
        int vm_socket = create_socket(vm_client.ip_address,vm_client.port,vm_client.user_type);


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
                   consist of the sock veriable in case a new connection
                   is coming in, plus all the sockets we have already
                   accepted. */
                FD_ZERO(&socks);
                /* Loops through all the possible connections and adds those sockets to the fd_set */
                // Note that one thread handles only 2 sockets, one from the  client, the other to the VM!

                for (listnum = 0; listnum < 2; listnum++) {
                        if (connectlist[listnum] != 0) {
                                FD_SET(connectlist[listnum],&socks);
                                if (connectlist[listnum] > highsock) {
                                        highsock = connectlist[listnum];
                                }
                        }
                }

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
					//printf("Writing to client on socket: %d on actual_index: %d\n", client_socket, actual_index[0]);
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
				free(vm_client_arg);
				pthread_exit(NULL);
			}
	                
       			if(transferred_bytes > 0){
				append_buffer(buffer_from_client,aux_buffer_from_client,&bytes_ready_from_client,transferred_bytes,&times);	
			}	
		
			if(bytes_ready_from_client > 0) {
				//printf("Read from client on socket %d:\n%s\n---\n",client_socket, (char *)buffer_from_client);
				//printf("Read from client on socket: %d on actual_index: %d\n", client_socket, actual_index[0]);
				if(!vm_client.user_type)
					lambda++;
			}	
				
		}
		// Check if the VM is ready
		if(FD_ISSET(vm_socket, &socks)) {
			// First of all, check if we have something for the VM to send
			if(bytes_ready_from_client > 0) {
				//printf("Writing to VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_from_client);
				//printf("Writing to VM on socket: %d on actual_index: %d\n", vm_socket, actual_index[0]);
					
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
i			free(buffer_from_client);
			free(buffer_to_client);
			free(aux_buffer_from_client);
			free(aux_buffer_to_client);
			close(vm_socket);
			close(client_socket);
			free(vm_client_arg);	
			pthread_exit(NULL);
		}
	                
		if(transferred_bytes > 0){
					append_buffer(buffer_to_client,aux_buffer_to_client,&bytes_ready_to_client,transferred_bytes,&times);
				}
				
				if(bytes_ready_to_client > 0) {
					//printf("Read from VM on socket %d:\n%s\n---\n",vm_socket, (char *)buffer_to_client);
					//printf("Read from VM on socket: %d on actual_index: %d\n", vm_socket, actual_index[0]);
				}

			}

                }
        }

}


void add_vm(struct virtual_machine * vm) {
	struct virtual_machine *new_vm=(struct virtual_machine *)malloc(sizeof (struct virtual_machine));
	memcpy(new_vm, vm, sizeof(struct virtual_machine));
	if (vm_list == NULL) {
		vm_list = vm;
	} else {
		struct vm_list_elem* vm_list_temp = vm_list;
		while (vm_list_temp->next) {
			vm_list_temp = vm_list_temp->next;
		}
		vm_list_temp->next = vm;
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
	
	struct virtual_machine vm;
	
	printf("Waiting for communication by the controller...\n");
	while(1){
		// Wait for info by the controller
		if ((recv(socket, &vm, sizeof(struct virtual_machine),0)) == -1){
				perror("Error while receiving data from controller");
				close(socket);
		}
		printf("New command received by controller for vm <%s,%d>, operation: %d\n", vm.ip, vm.port, vm.op);
	
		// It is a pointer to a vm_data struct in the vm_data_set
		struct vm_data * temp_vm_data;
		temp_vm_data = &vm_data_set[vm.service][current_vms[vm.service]];
		
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
		current_vms[index] = 0;
		actual_index[index] = 0;
		// we need the second control because malloc may return NULL even if its argument is zero
		if((vm_data_set[index] = malloc(sizeof(struct vm_data)*NUMBER_VMs)) == NULL && (NUMBER_VMs != 0))
			exit(EXIT_FAILURE);
		allocated_vms[index] = NUMBER_VMs;
	}
}

void get_my_own_ip(){
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
    if(f == NULL)
        abort();

    //fgets(my_own_ip, 16, f);
    fscanf(f,"%s",my_own_ip);
    pclose(f);
}

void * accept_balancers(void * v){
	while(1) {

                int connection;
		struct sockaddr_in client;
                unsigned int addr_len;
                addr_len = sizeof(struct sockaddr_in);
                connection = accept(socket_remote_balancer, (struct sockaddr *)&client, &addr_len);
                if (connection < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }
                setnonblocking(connection);

                struct arg_thread *vm_client=(struct arg_thread*)malloc(sizeof(struct arg_thread));

                vm_client->socket = connection;
                strcpy(vm_client->ip_address,inet_ntoa(client.sin_addr));
                vm_client->port = ntohs(client.sin_port);
		vm_client->user_type = 1;		

                printf("New balancer connected from <%s, %d>\n", vm_client->ip_address, vm_client->port);
                res_thread = create_thread(connection_thread, vm_client);

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
	int sockfd_controller_arrival_rate;
	int port_arrival_rate;
	
	pthread_attr_t pthread_custom_attr;
	pthread_t tid;
	pthread_t tid_arrival_rate;
	pthread_t tid_balancer;
	
	// Service_name: used to collect all the info about the forwarder net infos
	// ip_controller: ip used to contact the controller
	// port_controller: port number used to contact the controller
	if (argc != 7) {
		printf("Usage: %s Service_name ip_controller port_controller port_controller_arrival_rate port_tpcw port_remote_lb\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	
	vm_list=0;
	port_arrival_rate = atoi(argv[4]);
	port_remote_balancer = atoi(argv[6]);
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
	if (connect(sockfd_controller, (struct sockaddr *)&controller , sizeof(controller)) < 0) {
            perror("main: connect_to_controller");
            exit(EXIT_FAILURE);
        }
	// Send to controller balancer public ip address
	get_my_own_ip();
	if(sock_write(sockfd_controller, my_own_ip, 16) < 0){
		perror("Error in sending balancer public ip address to its own controller: ");
	}
        printf("Balancer %s correctely connected to its own controller %s on port %d\n", my_own_ip, inet_ntoa(controller.sin_addr), ntohs(controller.sin_port));

	// Once connection is created, build up a new thread to implement the exchange of messages between LB and Controller
	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&tid,&pthread_custom_attr,controller_thread,(void *)(long)sockfd_controller);
	
	/* CONNECTION LB - CONTROLLER ARRIVAL RATE */
	printf("Creating socket to controller arrival rate...\n");
	sockfd_controller_arrival_rate = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd_controller_arrival_rate < 0) {
		perror("main: socket_controller_arrival_rate");
		exit(EXIT_FAILURE);
	}
	// Controller's info
	controller.sin_family = AF_INET;
	controller.sin_addr.s_addr = inet_addr(argv[2]);
	controller.sin_port = htons(port_arrival_rate);
	
	// Connect to controller (it if (bind(sock, (struct sockaddr *) &server_address,
	if (connect(sockfd_controller_arrival_rate, (struct sockaddr *)&controller , sizeof(controller)) < 0) {
        perror("main: connect_to_controller arrival rate");
        exit(EXIT_FAILURE);
    }
    printf("Correctely connected to controller %s on port %d\n", inet_ntoa(controller.sin_addr), port_arrival_rate);

	memset(regions,0,sizeof(struct _region)*NUMBER_REGIONS);
	// Once connection is created, build up a new thread to implement the exchange of messages between LB and Controller
	pthread_attr_init(&pthread_custom_attr);
	timer_start(arrival_rate_timer);
	pthread_create(&tid_arrival_rate,&pthread_custom_attr,update_region_features,(void *)(long)sockfd_controller_arrival_rate);
	
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
	port = atoi(argv[5]); //sockethelp.c
	memset((char *) &server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);
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
          sizeof(balancer_address)) < 0 ) {
                perror("bind");
                close(socket_remote_balancer);
                exit(EXIT_FAILURE);
        }

        if(listen(socket_remote_balancer, MAX_NUM_OF_CLIENTS) < 0){
                perror("listen: ");
        }
        if(listen(socket_remote_balancer, MAX_NUM_OF_CLIENTS) < 0){
                perror("listen: ");
        }
        printf("Listening on port for incoming connection from other balancers %d\n", port_remote_balancer);
	pthread_create(&tid_balancer,&pthread_custom_attr,accept_balancers,NULL);

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
		if(!current_vms[0]){
			close(connection); 
			continue;
		}
		//printf("accepted connection on sockid %d from client %s\n", connection, inet_ntoa(client.sin_addr));

		struct arg_thread *vm_client=(struct arg_thread*)malloc(sizeof(struct arg_thread));
		
		vm_client->socket = connection;
		strcpy(vm_client->ip_address,inet_ntoa(client.sin_addr));
		vm_client->port = ntohs(client.sin_port);
		vm_client->user_type = 0;		

		//printf("New client connected from <%s, %d>\n", vm_client.ip_address, vm_client.port);
		res_thread = create_thread(connection_thread, vm_client);

	}
}
