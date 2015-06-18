#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "commands.h"
#include "ml_models.h"
#include "system_features.h"
#include <ifaddrs.h>
#include <arpa/inet.h>


#define COMMUNICATION_TIMEOUT   60
#define NUMBER_GROUPS           3       // one group for each type of service
#define NUMBER_VMs              1024       // number of possible VMs initially for each group
#define CONN_BACKLOG            1024    // max number of pending connections
#define BUFSIZE                 4096    // buffer size (in bytes)
#define TTC_THRESHOLD           300     // threshold to rejuvenate the VM (in sec)

#define MTTF_SLEEP				10		// avg rej rate period
#define PATH 					"/home/luca/Scrivania/controllers_list.txt"
#define GLOBAL_CONTROLLER_PORT	4567

void send_command_to_load_balancer();

int ml_model;                           // used machine-learning model
struct timeval communication_timeout;
int current_vms[NUMBER_GROUPS];         // number of connected VMs to the server
int allocated_vms[NUMBER_GROUPS];       // number of allocated spaces for the VMs (current_vms[i] <= allocated_vms[i])
pthread_mutex_t mutex;                  // mutex used to update current_vms and allocated_vms values
int state_man_vm = 0;
pthread_mutex_t manage_vm_mutex;
pthread_mutex_t lb_mutex;
pthread_mutex_t mttf_mutex;
float * rej_rate;
int index_rej_rate = 0;
int sockfd_balancer;	//socket number for Load Balancer (LB)

/*** TODO: initialize with real provided services ***/
enum operations{
	ADD, DELETE, REJ
};

// services provided
enum services {
    SERVICE_1, SERVICE_2, SERVICE_3
};

// STAND_BY has value 0, ACTIVE has value 1, REJUVENATING has value 2, and so on...
enum vm_state {
    STAND_BY, ACTIVE, REJUVENATING
};

// service info for each CN
struct vm_service{
    enum services service;
    volatile enum vm_state state;
    int provided_port;
};

typedef struct _vm_data {
    int socket;
    char ip_address[16];
    int port;
    volatile struct vm_service service_info;
    system_features last_features;
    int last_system_features_stored;
}vm_data;

vm_data ** vm_data_set[NUMBER_GROUPS]; //Groups of VMs

struct virtual_machine{
	char ip[16];
	int port;
	enum services service;
	enum operations op;
};

struct virtual_machine virt_machine;

/* Fill current_features */
void get_feature(char *features, system_features current_features) {
    sscanf(features, "Datapoint: %f %d\nMemory: %d %d %d %d %d %d\nSwap: %d %d %d\nCPU: %f %f %f %f %f %f",
           &current_features.time, &current_features.n_th,
           &current_features.mem_total,&current_features.mem_used,&current_features.mem_free,&current_features.mem_shared,&current_features.mem_buffers,&current_features.mem_cached,
           &current_features.swap_total,&current_features.swap_used,&current_features.swap_free,
           &current_features.cpu_user,&current_features.cpu_nice,&current_features.cpu_system,&current_features.cpu_iowait,&current_features.cpu_steal,&current_features.cpu_idle);
}

void store_last_system_features(system_features *last_features, system_features current_features) {
    memcpy(last_features, &current_features, sizeof(system_features));
}

/* This function compacts groups in vm_data_set 
 * when there are closed connections
 */
/*
void compact_vm_data_set(vm_data vm){
    int index;
    int group;
    group = vm.service_info.service;
    for (index = 0; index < current_vms[group]; index++) {
        if (strcmp(vm_data_set[group][index]->ip_address, vm.ip_address) == 0) {
            vm_data_set[group][index] = vm_data_set[group][--current_vms[group]];
            break;
        }
    }
}*/

void switch_crashed_machine(vm_data vm) {
    int index;
    char send_buff[BUFSIZE];
    
    for (index = 0; index < allocated_vms[vm.service_info.service]; index++) {
		if (vm_data_set[vm.service_info.service][index] != NULL){
			if (vm_data_set[vm.service_info.service][index]->service_info.state == STAND_BY) {
				vm_data_set[vm.service_info.service][index]->service_info.state = ACTIVE;
				//printf("SWITCH POINTER %i VM with ip address %s and sock number %d activated\n", vm_data_set[vm.service_info.service][index], vm_data_set[vm.service_info.service][index].ip_address, vm_data_set[vm.service_info.service][index].socket);
				printf("SWITCH POINTER %p\n", &(vm_data_set[vm.service_info.service][index]->service_info.state));
				printf("VM of group %d in position %d activated\n", vm.service_info.service, index);
				strcpy(virt_machine.ip,vm_data_set[vm.service_info.service][index]->ip_address);
				virt_machine.port = htons(8080); //TODO
				virt_machine.service = vm.service_info.service;
				virt_machine.op = ADD;
				send_command_to_load_balancer();
            
				break;
			}
		}
    }
}

/* This function looks for a standby VM and 
 * attempts to active it to replace a VM 
 * in rejuvenation state
 */
void switch_active_machine(vm_data vm) {
    int index;
    char send_buff[BUFSIZE];
    
    //pthread_mutex_lock(&mutex);
    vm.service_info.state = REJUVENATING;
    for (index = 0; index < current_vms[vm.service_info.service]; index++) {
        if (vm_data_set[vm.service_info.service][index]->service_info.state == STAND_BY) {
            vm_data_set[vm.service_info.service][index]->service_info.state = ACTIVE;
			//printf("SWITCH POINTER %i VM with ip address %s and sock number %d activated\n", vm_data_set[vm.service_info.service][index], vm_data_set[vm.service_info.service][index].ip_address, vm_data_set[vm.service_info.service][index].socket);
            printf("SWITCH POINTER %p\n", &(vm_data_set[vm.service_info.service][index]->service_info.state));
            printf("VM of group %d in position %d activated\n", vm.service_info.service, index);
            strcpy(virt_machine.ip,vm_data_set[vm.service_info.service][index]->ip_address);
            virt_machine.port = htons(8080); //TODO
            virt_machine.service = vm.service_info.service;
            virt_machine.op = ADD;
            send_command_to_load_balancer();
            
            break;
        }
    }
    //pthread_mutex_unlock(&mutex);
    vm.last_system_features_stored = 0;
    /*** ASK: notify load balancer with "index" value here? ***/
    
    bzero(send_buff, BUFSIZE);
    send_buff[0] = REJUVENATE;
    if ((send(vm.socket, send_buff, BUFSIZE, 0)) == -1) {
        perror("switch_active_machine - send");
        printf("Closing connection with VM %s\n", vm.ip_address);
        //compact_vm_data_set(vm);
        close(vm.socket);
    } else{
        printf("REJUVENATE command sent to machine with IP address %s\n", vm.ip_address);
        fflush(stdout);
    }
    
}
/*
void * balancer_thread(void * v){
	int socket;
	socket = (int)(long)v;
	
	while(1){
		if(state_man_vm == 1){
			if(send(socket,&virt_machine,sizeof(struct virtual_machine),0) < 0){
				perror("balancer_thread: send");
			}
			printf("Messaggio inviato correttamente al balancer\n");
			state_man_vm = 0;
			pthread_mutex_unlock(&manage_vm_mutex);
			
			sleep(1);
		}
	}
	
}*/

void send_command_to_load_balancer(){

	if(send(sockfd_balancer,&virt_machine,sizeof(struct virtual_machine),0) < 0){
		perror("Error while sending command to load balancer");
	}
	printf("Command sent to load balancer\n");
}

void * mttf_thread(void * args){
	
	int index;
	float avg_rej_rate;
	int thread_index_rej_rate;
	
	while(1){
	
		if(index_rej_rate == 0)
			continue;
	
		avg_rej_rate = 0;
	
		pthread_mutex_lock(&mttf_mutex);
		thread_index_rej_rate = index_rej_rate;
		for(index = 0; index < thread_index_rej_rate; index++)
			avg_rej_rate += rej_rate[index];
		index_rej_rate = 0;
		pthread_mutex_unlock(&mttf_mutex);
	
		avg_rej_rate = avg_rej_rate/thread_index_rej_rate;
		
		printf("AVG_REJ_RATE %f\n", avg_rej_rate);
		//TODO: prepare the message for the broadcaster
	
		sleep(MTTF_SLEEP);
	}
}

/* THREAD FUNCTION
 * The duties of this thread are ONLY to listen for messages
 * from a connected VM. If something goes wrong this thread
 * should put a request for its own removal in the incoming
 * requests queue.
 */
void * communication_thread(void * v){

	vm_data ** pointer_to_vm_pointer = (vm_data **)v;
    vm_data * vm = (vm_data *)*pointer_to_vm_pointer;
    //vm = (vm_data *)(*v);   // this is exactly the struct stored in the system matrix

    //vm_data * vm = (vm_data *)malloc(sizeof(vm_data));
    //memcpy(vm,v,sizeof(vm_data));

    int numbytes;
    system_features current_features;
    system_features init_features;
    int flag_init_features = 0;
    char recv_buff[BUFSIZE];
    char send_buff[BUFSIZE];

    while (1){
		printf("Communication_thread socket for VM %s is: %d\n", vm->ip_address, vm->socket);
        if (vm->service_info.state != ACTIVE){
				//printf("STANDBY VM POINTER %i Sock number of Standby VM %d and ip address %s\n", &vm, vm.socket, vm.ip_address);
				printf("STANDBY POINTER %p\n", &(vm->service_info.state));
				fflush(stdout);
			}else {
            printf("Waiting for features from the CN %s for the service: %d\n", vm->ip_address, vm->service_info.service);
            fflush(stdout);
            bzero(recv_buff,BUFSIZE);
            // check recv features
            //if ((numbytes = recv(vm->socket,recv_buff,BUFSIZE,0)) == -1) {
			if ((numbytes = sock_read(vm->socket,recv_buff,BUFSIZE)) == -1) {
                printf("Failed receiving data from the VM %s for the service with sockid %d: %d\n", vm->ip_address, vm->socket, vm->service_info.service);
                perror("recv: ");
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("Timeout on recv() while waiting data from VM %s\n", vm->ip_address);
                }
                break;
            } else  if (numbytes == 0) {
                printf("VM %s is disconnected\n", vm->ip_address);
                break;
            } else{
                // features correctly received
                printf("Received data (%d bytes) from VM %s for the service %d\n", numbytes, vm->ip_address, vm->service_info.service);
                printf("%s\n", recv_buff);
                
                fflush(stdout);
                get_feature(recv_buff, current_features);
                
                // fill init_features only the first time
                if(!flag_init_features){
					memcpy(&init_features,&current_features,sizeof(system_features));
					flag_init_features = 1;
				}
				
                // at least 2 sets of features needed
                if (vm->last_system_features_stored) {
					float mean_time_to_fail = get_predicted_mttf(ml_model, vm->last_features, current_features, init_features);
					pthread_mutex_lock(&mttf_mutex);
					rej_rate[index_rej_rate++] = (1/mean_time_to_fail);
					pthread_mutex_unlock(&mttf_mutex);
                    //float predicted_time_to_crash = get_predicted_rttc(ml_model, vm.last_features, current_features);
                    float predicted_time_to_crash = 1000;
                    printf("Predicted time to crash for VM %s for the service %d is: %f\n", vm->ip_address, vm->service_info.service, predicted_time_to_crash);
                    if (predicted_time_to_crash < (float)TTC_THRESHOLD) {
                        
                        //pthread_mutex_lock(&manage_vm_mutex);
                        pthread_mutex_lock(&mutex);
                        
                        switch_active_machine(*vm);
                        
                        strcpy(virt_machine.ip,vm->ip_address);
                        virt_machine.port = htons(8080); //TODO
                        virt_machine.service = vm->service_info.service;
                        virt_machine.op = REJ;
                        send_command_to_load_balancer();
                        pthread_mutex_lock(&mutex);
                        //state_man_vm = 1;
                        continue;
                    }
                }
                store_last_system_features(&(vm->last_features),current_features);
                vm->last_system_features_stored = 1;
                //sending CONTINUE command to the VM
                bzero(send_buff, BUFSIZE);
                send_buff[0] = CONTINUE;
                if ((send(vm->socket, send_buff, BUFSIZE,0)) == -1) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        printf("Timeout on send() while sending data by VM %s\n", vm->ip_address);
                        fflush(stdout);
                    } else {
                        printf("Error on send() while sending data by VM %s\n", vm->ip_address);
                        fflush(stdout);
                    }
                    break;
                }
            }
        }
        sleep(1);
    }
    
    printf("Closing connection with VM %s\n", vm->ip_address);
    pthread_mutex_lock(&mutex);
    
    strcpy(virt_machine.ip,vm->ip_address);
    virt_machine.port = htons(8080);
    virt_machine.service = vm->service_info.service;
    virt_machine.op = DELETE;
    
    //state_man_vm = 1;
    send_command_to_load_balancer();
    //compact_vm_data_set(*vm);
    
    switch_crashed_machine(*vm);
    
    if(close(vm->socket) == 0){
		printf("Connection correctely closed with VM %s\n", vm->ip_address);
	}
	pthread_mutex_unlock(&mutex);
	memset(pointer_to_vm_pointer,0,sizeof(vm_data *));
	pthread_exit(0);
}

/*
 * Listens for incoming connection requests. When such arrives this void
 * fires a new thread to handle it.
 * sockfd - the socketd address of the server
 * pthread_custom_attr - standart attributes for a thread
 */
void accept_new_client(int sockfd, pthread_attr_t pthread_custom_attr){
    unsigned int addr_len;
    int socket;
    int numbytes;
    pthread_t tid;
    int index;
    char * buffer;
    buffer = malloc(BUFSIZE);
    
    // store here infos from CNs
    struct vm_service s;
    
    struct sockaddr_in client;
    addr_len = sizeof(struct sockaddr_in);
    
    // get the first pending VM connection request
    printf("Accepting client\n");
    if ((socket = accept(sockfd, (struct sockaddr *) &client, &addr_len)) == -1) {
        perror("accept_new_client - accept");
    }
    
    /*** ASK: should we set with different timeout for "handshake" phase? ***/
    // set socket timeout
    if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    
    // wait for info about service provided by the VM
    // if errors occur close the socket
    printf("Waiting for info by client\n");
    if ((numbytes = recv(socket,&s, sizeof(struct vm_service),0)) == -1) {
        perror("accept_new_client - recv");
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            perror("accept_new_client - recv timeout");
            close(socket);
        }
    } else if(numbytes == 0){
        perror("accept_new_client - disconnected VM");
        close(socket);
    } else{
        //control that CN provide an existing service
        while(1){
			bzero(buffer,BUFSIZE);
			if(s.service >= NUMBER_GROUPS){
				sprintf(buffer,"groups");
			}
			else sprintf(buffer,"ok");
			if((send(socket,buffer,BUFSIZE,0)) == -1){
				perror("accept_new_client - send");
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					perror("accept_new_client - send timeout");
					fflush(stdout);
				}
				close(socket);
			}
			if(!strcmp(buffer,"ok")){
				break;
			}
			if((numbytes = recv(socket,&s,sizeof(struct vm_service),0)) == -1){
				perror("accept_new_client - recv");
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					perror("accept_new_client - recv timeout");
					close(socket);
				}
			} else if(numbytes == 0){
				perror("accept_new_client - disconnected VM");
				close(socket);
			}
		}
        
        // increment number of connected CNs
        pthread_mutex_lock(&mutex);
        
        int current_position = 0;
        while(vm_data_set[s.service][current_position] != NULL){
			current_position++;
			if(current_position == allocated_vms[s.service]){
				vm_data_set[s.service] = realloc(vm_data_set[s.service],sizeof(vm_data *)*2*allocated_vms[s.service]);
				memset(vm_data_set[s.service][current_position], 0, sizeof(vm_data *)*allocated_vms[s.service]);
			
				allocated_vms[s.service] *= 2;
			}
		}

        // allocate more slots if the memory is not enough
        /*if (current_vms[s.service] == allocated_vms[s.service]) {
            vm_data_set[s.service] = realloc(vm_data_set[s.service],sizeof(vm_data)*2*allocated_vms[s.service]);
            
            int previously_allocated = allocated_vms[s.service];
            
            allocated_vms[s.service] *= 2;
            
            rej_rate = realloc(rej_rate,sizeof(float)*NUMBER_GROUPS*NUMBER_VMs + allocated_vms[s.service] - previously_allocated);
        }*/
               
        // assignment phase
        vm_data * new_vm = (vm_data *)malloc(sizeof(vm_data));
        new_vm->service_info = s;
        strcpy(new_vm->ip_address, inet_ntoa(client.sin_addr));
        new_vm->socket = socket;
        new_vm->port = ntohs(client.sin_port);
        
        vm_data_set[s.service][current_position] = new_vm;
                
        if(s.state == ACTIVE){
			
			//pthread_mutex_lock(&lb_mutex);
			strcpy(virt_machine.ip,inet_ntoa(client.sin_addr));
			virt_machine.port = htons(8080);
			virt_machine.service = s.service;
			virt_machine.op = ADD;
			//this flag notifies that the Controller want to communicate with LB
			send_command_to_load_balancer();
			
		}

        // make a new thread for each VMs
        printf("New VM with IP address %s added in group %d in position %d sockid %d - %d\n", new_vm->ip_address, s.service, current_position, socket, new_vm->socket);
        pthread_attr_init(&pthread_custom_attr);
        if(pthread_create(&tid,&pthread_custom_attr,communication_thread,(void *)&vm_data_set[s.service][current_position]) != 0){
			perror("Error on pthread_create while accepting new client");
		}
		pthread_mutex_unlock(&mutex);
    }
    
}
/*
 * This function accepts the connection incoming from the balancer
 * it must to return -1 if something wrong
 * or it must to return 0 if communication with balancer is established
 */
int accept_load_balancer(int sockfd, pthread_attr_t pthread_custom_attr){
	unsigned int addr_len;
    int socket;
    int numbytes;
    pthread_t tid;
    int index;
    int n;
    
    struct sockaddr_in balancer;
    addr_len = sizeof(struct sockaddr_in);
    
    // get the first pending VM connection request
    printf("Accepting load_balancer\n");
    if ((socket = accept(sockfd, (struct sockaddr *) &balancer, &addr_len)) == -1) {
        perror("accept_load_balancer - accept");
        return (int)-1;
    }
    
    /*** ASK: should we set with different timeout for "handshake" phase? ***/
    // set socket timeout
    if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    if (setsockopt (socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&communication_timeout, sizeof(communication_timeout)) < 0)
        printf("Setsockopt failed for socket id %i\n", socket);
    sockfd_balancer = socket;
    // make a new thread for each VMs
	printf("Communication with load_balancer %s established\n", inet_ntoa(balancer.sin_addr));
}

/*
void start_server_dgram(int * sockfd){
	int sock;
	struct sockaddr_in temp;
	
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("start_server_dgram - socket");
		exit(1);
	}
	
	temp.sin_family = AF_INET;
	temp.sin_addr.s_addr = htonl(INADDR_ANY);
	temp.sin_port = htons(port);
	
	if(bind(sock,(struct sockaddr *) &temp, sizeof(temp)) <0){
		perror("start_server_dgram - bind");
		exit(1);
	}
}*/

/*
 * This function opens the VMCI server for connections;
 * sockfd_addr - the place at which the server socket
 * address is written.
 */
void start_server(int * sockfd, int port){
    int sock;
    struct sockaddr_in temp;
    
    //Create socket
    if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("start_server - socket");
        exit(1);
    }
    //Server address
    temp.sin_family = AF_INET;
    temp.sin_addr.s_addr = htonl(INADDR_ANY);
    temp.sin_port = htons(port);
    //Bind
    if (bind(sock, (struct sockaddr*) &temp, sizeof(temp))) {
        perror("start_server - bind");
        exit(1);
    }
    //Listen for max CONN_BACKLOG
    if (listen(sock, CONN_BACKLOG)) {
        perror("start_server - listen");
        exit(1);
    }
    *sockfd = sock;
}


int main(int argc,char ** argv){
    int sockfd;				//socket number for Computing Nodes (CN)
    int port;				//port number for CN
    int port_balancer;		//port number for LB
    int index;
    
    int sock_dgram;
    
    pthread_attr_t pthread_custom_attr;
    pthread_t tid;
    
    communication_timeout.tv_sec = COMMUNICATION_TIMEOUT;
    communication_timeout.tv_usec = 0;
    
    if (argc != 4) {
        /*** TODO: added argv[0] to avoid warning caused by %s ***/
        printf("Usage: %s vm_port_number load_balancer_port_number ml_model_number\n", argv[0]);
        exit(1);
    }
    else if (atoi(argv[1]) == atoi(argv[2])) {
		printf("Port numbers must be different! Both the ports have the same port number %d\n", atoi(argv[1]));
		exit(1);
	}
    
    port = atoi(argv[1]);
    port_balancer = atoi(argv[2]);
    ml_model = atoi(argv[3]);
    
    //Allocating initial memory for the VMs
    /*** ATTENZIONE : Problema con l'allocazione della malloc, mi permette di scrivere in altre zone di memoria di almeno 4B ***/
    for (index = 0; index < NUMBER_GROUPS; index++) {
        current_vms[index] = 0;	// actually no VMs are connected yet
        vm_data_set[index] = malloc(sizeof(vm_data *)*NUMBER_VMs); // allocate the same memory initially for each groups
        allocated_vms[index] = NUMBER_VMs;  // number of slots allocated for each group
        
        memset(vm_data_set[index], 0, sizeof(vm_data *)*NUMBER_VMs);
        
        rej_rate = (float *)malloc(sizeof(float)*NUMBER_GROUPS*NUMBER_VMs);
    }
    
    //Open the local connection
    start_server(&sockfd,port);
    printf("Server for VMs started, listening on socket %d on port %d\n", sockfd, port);
    //Open the connection with the load_balancer
    start_server(&sockfd_balancer,port_balancer);
    printf("Server for load balancer started, listening on socket %d on port %d\n", sockfd_balancer, port_balancer);
    
    //Start dedicated thread to communicate with LB
    //It must block until the system is not ready
    if((accept_load_balancer(sockfd_balancer,pthread_custom_attr)) < 0)
		exit(1);
    
    pthread_attr_init(&pthread_custom_attr);
    pthread_create(&tid,&pthread_custom_attr,mttf_thread,NULL);
    
	//start_server_dgram(&sock_dgram);
    
    //Init of broadcast and leader primitives
    //initialize_broadcast(PATH);
    //initialize_leader(PATH);
    
    //Accept new clients
    while(1){
        accept_new_client(sockfd,pthread_custom_attr);
    }
    
}
