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
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <time.h>
#include "flow_function.h"
#include "vm_list.h"

#define COMMUNICATION_TIMEOUT   60
#define NUMBER_GROUPS           3       // one group for each type of service
#define NUMBER_VMs              1024       // number of possible VMs initially for each group
#define NUMBER_REGIONS			4
#define CONN_BACKLOG            1024    // max number of pending connections
#define BUFSIZE                 4096    // buffer size (in bytes)
#define TTC_THRESHOLD           200     // threshold to rejuvenate the VM (in sec)

#define MTTF_SLEEP				10		// avg rej rate period
#define PATH 					"./controllers_list.txt"
#define GLOBAL_CONTROLLER_PORT	4567

void send_command_to_load_balancer();
void get_my_own_ip();
void update_region_workload_distribution();

int ml_model;                           // used machine-learning model
int number_of_active_vm;				//number of vms to be activated;
struct timeval communication_timeout;
pthread_mutex_t mutex; // mutex used to update current_vms and allocated_vms values
int state_man_vm = 0;
int index_rej_rate = 0;
int sockfd_balancer;	//socket number for Load Balancer (LB)
int sockfd_balancer_arrival_rate;
float region_mttf;
int i_am_leader = 0;
char leader_ip[16];
int socket_controller_communication;
char my_own_ip[16];
char my_balancer_ip[16];
float global_flow_matrix[NUMBER_REGIONS][NUMBER_REGIONS];

struct vm_list_elem *vm_list;

struct _region regions[NUMBER_REGIONS];

struct virtual_machine_operation vm_op;

/* Fill current_features */
void get_feature(char *features, system_features *current_features) {
	sscanf(features,
			"Datapoint: %f %d\nMemory: %d %d %d %d %d %d\nSwap: %d %d %d\nCPU: %f %f %f %f %f %f",
			&current_features->time, &current_features->n_th,
			&current_features->mem_total, &current_features->mem_used,
			&current_features->mem_free, &current_features->mem_shared,
			&current_features->mem_buffers, &current_features->mem_cached,
			&current_features->swap_total, &current_features->swap_used,
			&current_features->swap_free, &current_features->cpu_user,
			&current_features->cpu_nice, &current_features->cpu_system,
			&current_features->cpu_iowait, &current_features->cpu_steal,
			&current_features->cpu_idle);
}

void store_last_system_features(system_features *last_features,
		system_features current_features) {
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

void activate_new_machine() {
	int index;
	char send_buff[BUFSIZE];

	for (index = 0; index < vm_list_size(vm_list); index++) {
		struct virtual_machine *vm = get_vm_by_position(index, vm_list);
		if (vm->state == STAND_BY) {
			vm->state = ACTIVE;
			strcpy(vm_op.ip, vm->ip);
			vm_op.port = htons(8080); //TODO
			//virt_machine.service = SERVICE_0;
			vm_op.op = ADD;
			send_command_to_load_balancer();
			printf("Activated vm with ip: %s\n", vm->ip);
			return;
		}
	}
	printf("No vms available to be activeted\n");
}

/* This function looks for a standby VM and 
 * attempts to active it to replace a VM 
 * in rejuvenation state
 */
void switch_active_machine(struct virtual_machine *vm) {
	int index;
	char send_buff[BUFSIZE];

	strcpy(vm_op.ip, vm->ip);
	vm_op.port = htons(8080); //TODO
	vm_op.op = DELETE;
	send_command_to_load_balancer();

	for (index = 0; index < vm_list_size(vm_list); index++) {
		struct virtual_machine *vm = get_vm_by_position(index, vm_list);
		if (vm->state == STAND_BY) {
			vm->state = ACTIVE;
			printf("Activated vm with ip: %s\n", vm->ip);
			strcpy(vm_op.ip, vm->ip);
			vm_op.port = htons(8080); //TODO
			//virt_machine.service = SERVICE_0;
			vm_op.op = ADD;
			send_command_to_load_balancer();
			break;
		}
	}
	vm->last_system_features_stored = 0;
	/*** ASK: notify load balancer with "index" value here? ***/

	bzero(send_buff, BUFSIZE);
	send_buff[0] = REJUVENATE;
	if ((send(vm->socket, send_buff, BUFSIZE, 0)) == -1) {
		perror("switch_active_machine - send");
		printf("Closing connection with VM %s\n", vm->ip);
		//compact_vm_data_set(vm);
		close(vm->socket);
	} else {
		printf("REJUVENATE command sent to machine with IP address %s\n",
				vm->ip);
		fflush(stdout);
	}

}

void send_command_to_load_balancer() {

	if (send(sockfd_balancer, &vm_op, sizeof(struct virtual_machine_operation),
			0) < 0) {
		perror("Error while sending command to load balancer");
	}
	int i;
	printf("Command sent to load balancer\n");
}

void compute_region_mttf() {
	float mttfs = 0;
	int number_active_vms = 0;

	int index;
	int index_2;

	for (index = 0; index < vm_list_size(vm_list); index++) {
		struct virtual_machine *vm = get_vm_by_position(index, vm_list);
		if (vm->state == ACTIVE && vm->mttf > 0) {
			mttfs = mttfs + vm->mttf;
			number_active_vms++;
		}
	}
	printf("Total mttf is %f on %d active vms. ", mttfs, number_active_vms);
	region_mttf = mttfs / number_active_vms;
	printf("The region mttf value is %f\n", region_mttf);
}

void * update_region_features(void * arg) {
	int sockfd;
	sockfd = (int) (long) arg;
	struct _region temp;
	int index;

	while (1) {
		if (sock_read(sockfd, &temp, sizeof(struct _region)) < 0) {
			perror(
					"Error in reading from controller in update_region_features: ");
		}
		//printf("UPDATE_REGION_FEATURES: temp.ip_controller is %s\n", tFemp.ip_controller);
		pthread_mutex_lock(&mutex);
		for (index = 1; index < NUMBER_REGIONS; index++) {
			//printf("UPDATE_REGION_FEATURES: regions[%d].ip_controller is %s\n", index, regions[index].ip_controller);
			if (!strcmp(regions[index].ip_controller, temp.ip_controller)
					|| (strnlen(regions[index].ip_controller, 16) == 0)) {
				strcpy(regions[index].ip_controller, temp.ip_controller);
				strcpy(regions[index].ip_balancer, temp.ip_balancer);
				regions[index].region_features.arrival_rate =
						temp.region_features.arrival_rate;
				regions[index].region_features.mttf = temp.region_features.mttf;
				printf(
						"Received region features from controller %s with balancer %s with arrival_rate %f and mttf %f\n",
						regions[index].ip_controller,
						regions[index].ip_balancer,
						regions[index].region_features.arrival_rate,
						regions[index].region_features.mttf);
				update_region_workload_distribution();
				int i;
				for (i = 0; i < NUMBER_REGIONS; i++) {
					regions[i].probability = global_flow_matrix[index][i];
				}
				if (sock_write(sockfd, &regions,
				NUMBER_REGIONS * sizeof(struct _region)) < 0) {
					perror(
							"Error in sending the probabilities to all the other controllers: ");
				}
				printf("Probabilities correctly sent to controller %s\n",
						regions[index].ip_controller);
				break;
			}
		}
		pthread_mutex_unlock(&mutex);
	}
}

void * controller_communication_thread(void * v) {
	struct sockaddr_in incoming_controller;
	unsigned int addr_len = sizeof(incoming_controller);
	int sockfd;
	pthread_attr_t pthread_custom_attr;
	pthread_t tid;

	while (1) {
		if ((sockfd = accept(socket_controller_communication,
				(struct sockaddr *) &incoming_controller, &addr_len)) < 0) {
			perror("Error in accepting connections from other controllers: ");
		}

		pthread_attr_init(&pthread_custom_attr);
		pthread_create(&tid, &pthread_custom_attr, update_region_features,
				(void *) (long) sockfd);
	}
}

void update_region_workload_distribution() {
	float global_mttf = 0.0;
	float global_arrival_rate = 0.0;
	int index;
	int number_of_regions = 0;
	for (index = 0; index < NUMBER_REGIONS; index++) {
		if (strnlen(regions[index].ip_controller, 16) != 0
				&& !isnan(regions[index].region_features.mttf)) {
			global_mttf = global_mttf + regions[index].region_features.mttf;
			global_arrival_rate += regions[index].region_features.arrival_rate;
			//number_of_regions++;
		}
	}
	printf("Global arrival rate: %f\n", global_arrival_rate);
	printf("-----------------\nRegion distribution probabilities:\n");
	for (index = 0; index < NUMBER_REGIONS; index++) {
		if (strnlen(regions[index].ip_controller, 16) != 0) {
			if (isnan(regions[index].region_features.mttf)) {
				regions[index].probability = 0;
			} else if (isinf(regions[index].region_features.mttf)) {
				regions[index].probability = 1;
			} else {
				regions[index].probability = regions[index].region_features.mttf
						/ global_mttf;
			}

			printf(
					"Balancer %s, arrival rate: %f, mttf: %f, calculated request forwarding probability: %f\n",
					regions[index].ip_balancer,
					regions[index].region_features.arrival_rate,
					regions[index].region_features.mttf,
					regions[index].probability);
		}
	}
	printf("-----------------\n");

	float f[NUMBER_REGIONS], p[NUMBER_REGIONS];
	memset(f, 0, sizeof(float) * NUMBER_REGIONS);
	memset(p, 0, sizeof(float) * NUMBER_REGIONS);
	for (index = 0; index < NUMBER_REGIONS; index++) {
		if (global_arrival_rate != 0
				&& !isnan(regions[index].region_features.mttf)) {
			f[index] = regions[index].region_features.arrival_rate
					/ global_arrival_rate;
			p[index] = regions[index].probability;
		}
	}

	memset(global_flow_matrix, 0,
			sizeof(float) * NUMBER_REGIONS * NUMBER_REGIONS);
	calculate_flow_matrix(global_flow_matrix, f, p, NUMBER_REGIONS);
	printf("-----------------\nGlobal Flow Matrix:\n");
	print_matrix(global_flow_matrix, NUMBER_REGIONS);
	printf("-----------------\n");

        struct timeval the_timer;
        FILE *file;
        gettimeofday(&the_timer, 0);

        char tmbuf[64], buf[64];
        time_t now_time = the_timer.tv_sec;
        struct tm *now_tm;
        now_tm = localtime(&now_time);

        strftime(tmbuf, sizeof(tmbuf), "%H:%M:%S", now_tm);
        snprintf(buf, sizeof(buf), "%s", tmbuf);

        file = fopen("mttf_region_dump.txt", "a");
	for (index = 0; index < NUMBER_REGIONS; index++) {
		if (strnlen(regions[index].ip_balancer,16)!=0)
        		fprintf(file, "%s\tBalancer %s, arrival rate: %f, mttf: %f, calculated request forwarding probability: %f\n",
					buf, regions[index].ip_balancer, regions[index].region_features.arrival_rate, regions[index].region_features.mttf, regions[index].probability);
	}	
        fclose(file);

}

void send_regions_to_load_balancer(int sockfd) {
	if (sock_write(sockfd, &regions, sizeof(struct _region) * NUMBER_REGIONS)
			< 0) {
		perror("Error in sending regions to my own load balancer: ");
	}
}

void * get_region_features(void * sock) {

	int sockfd;
	sockfd = (int) (long) sock;

	float arrival_rate = 0;
	int connection;
	int index;

	//struct _region_features region_features;
	struct _region region;
	struct sockaddr_in balancer;
	unsigned int addr_len;
	addr_len = sizeof(struct sockaddr_in);

	/*if((connection = accept(sockfd, (struct sockaddr *)&balancer, &addr_len)) == -1){
	 perror("Error in receiving from LB in arrival_rate_thread: ");
	 exit(EXIT_FAILURE);
	 }*/
	strcpy(region.ip_balancer, my_balancer_ip);
	while (1) {
		if (sock_read(sockfd, &arrival_rate, sizeof(float)) < 0)
			perror("Error in reading in arrival_rate_thread: ");
		printf("Received arrival_rate lambda is: %.3f\n", arrival_rate);

		pthread_mutex_lock(&mutex);
		compute_region_mttf();

		if (!i_am_leader) {
			memset(regions, 0, NUMBER_REGIONS * sizeof(struct _region));
			get_my_own_ip();
			printf("GET_REGION_FEATURES: my_own_ip is %s\n", my_own_ip);
			strcpy(region.ip_controller, my_own_ip);
			region.region_features.arrival_rate = arrival_rate;
			region.region_features.mttf = region_mttf;
			if (sock_write(socket_controller_communication, &region,
					sizeof(struct _region)) < 0) {
				perror("Error in sending region features to leader: ");
			}
			if (sock_read(socket_controller_communication, &regions,
			NUMBER_REGIONS * sizeof(struct _region)) < 0) {
				perror("Error in receiving probabilities from leader: ");
			}

			printf("-----------------\nRegion distribution probabilities:\n");
			for (index = 0; index < NUMBER_REGIONS; index++) {
				if (strnlen(regions[index].ip_controller, 16) != 0) {
					printf("Balancer %s\t %f\n", regions[index].ip_balancer,
							regions[index].probability);
				}
			}
			printf("-----------------\n");

			if (sock_write(sockfd, &regions,
					sizeof(struct _region) * NUMBER_REGIONS) < 0) {
				perror("Error in sending regions to my own load balancer: ");
			}
		}
		//If i am leader, update my own values
		else {
			regions[0].region_features.arrival_rate = arrival_rate;
			regions[0].region_features.mttf = region_mttf;
			strcpy(regions[0].ip_balancer, my_balancer_ip);
			update_region_workload_distribution();

			int i;
			for (i = 0; i < NUMBER_REGIONS; i++) {
				regions[i].probability = global_flow_matrix[0][i];
			}
			if (sock_write(sockfd, &regions,
					sizeof(struct _region) * NUMBER_REGIONS) < 0) {
				perror("Error in sending regions to my own load balancer: ");
			}
		}
		pthread_mutex_unlock(&mutex);
	}
}

/* THREAD FUNCTION
 * The duties of this thread are ONLY to listen for messages
 * from a connected VM. If something goes wrong this thread
 * should put a request for its own removal in the incoming
 * requests queue.
 */
void * communication_thread(void * v) {

	struct virtual_machine *vm = (struct virtual_machine*) v;

	int numbytes;
	system_features current_features;
	system_features init_features;
	int flag_init_features = 0;
	char recv_buff[BUFSIZE];
	char send_buff[BUFSIZE];

	while (1) {
		fflush(stdout);
		bzero(recv_buff, BUFSIZE);
		// check recv features
		//if ((numbytes = recv(vm->socket,recv_buff,BUFSIZE,0)) == -1) {
		if ((numbytes = sock_read(vm->socket, recv_buff, BUFSIZE)) == -1) {
			printf("Failed receiving data from vm %s with sockid %i\n", vm->ip,
					vm->socket);
			perror("sock_read: ");
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				printf("Timeout on sock_read() while waiting data from VM %s\n",
						vm->ip);
			}
			break;
		} else if (numbytes == 0) {
			printf("vm %s is disconnected\n", vm->ip);
			break;
		}
		if (vm->state == ACTIVE) {

			fflush(stdout);
			get_feature(recv_buff, &current_features);

			// fill init_features only the first time
			if (!flag_init_features) {
				memcpy(&init_features, &current_features,
						sizeof(system_features));
				flag_init_features = 1;
			}

			// at least 2 sets of features needed
			if (vm->last_system_features_stored) {
				float mean_time_to_fail = get_predicted_mttf(ml_model,
						vm->last_features, current_features, init_features);
				vm->mttf = mean_time_to_fail;
				float predicted_time_to_crash = get_predicted_rttc(ml_model,
						vm->last_features, current_features);
				//float predicted_time_to_crash = 1000;
				printf("-----------------\nPredicted RTTC for vm %s: %f, predicted MTTF: %f \n",
						vm->ip, predicted_time_to_crash, mean_time_to_fail);

				//if (predicted_time_to_crash < (float)TTC_THRESHOLD) {
				if (0 && (double) rand() < (double) RAND_MAX / (double) 200) {
					vm->state == REJUVENATING;
					pthread_mutex_lock(&mutex);
					switch_active_machine(vm);
					pthread_mutex_unlock(&mutex);
					break;
				}
			}
		}
		store_last_system_features(&(vm->last_features), current_features);
		vm->last_system_features_stored = 1;
		//sending CONTINUE command to the VM
		bzero(send_buff, BUFSIZE);
		send_buff[0] = CONTINUE;
		if ((send(vm->socket, send_buff, BUFSIZE, 0)) == -1) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				printf("Timeout on send() while sending data by VM %s\n",
						vm->ip);
				fflush(stdout);
			} else {
				printf("Error on send() while sending data by VM %s\n", vm->ip);
				fflush(stdout);
			}
			break;
		}
		sleep(1);
	}

	if (close(vm->socket) == 0)
		printf("Connection correctly closed with VM %s\n", vm->ip);
		else
			printf("Error while closing connection with vm %s\n", vm->ip);

	pthread_mutex_lock(&mutex);
	remove_vm_by_ip(vm->ip, &vm_list);
	printf("-----------------\nCurrent vm list\n");
	print_vm_list(vm_list);
	if (vm->state == ACTIVE) {
		strcpy(vm_op.ip, vm->ip);
		vm_op.port = htons(8080);
		vm_op.op = DELETE;
		send_command_to_load_balancer();
	}
	if (get_number_of_active_vms(vm_list) < number_of_active_vm) {
		activate_new_machine();
		printf("-----------------\nCurrent vm list\n");
		print_vm_list(vm_list);
	}

	pthread_mutex_unlock(&mutex);
	pthread_exit(0);
}

/*
 * Listens for incoming connection requests. When such arrives this void
 * fires a new thread to handle it.
 * sockfd - the socketd address of the server
 * pthread_custom_attr - standart attributes for a thread
 */
void accept_new_client(int sockfd, pthread_attr_t pthread_custom_attr) {
	unsigned int addr_len;
	int socket;
	int numbytes;
	pthread_t tid;
	int index;
	char * buffer;
	buffer = malloc(BUFSIZE);

	// store here infos from CNs
	struct vm_service service;

	struct sockaddr_in client;
	addr_len = sizeof(struct sockaddr_in);

	// get the first pending VM connection request
	printf("Accepting client\n");
	if ((socket = accept(sockfd, (struct sockaddr *) &client, &addr_len))
			== -1) {
		perror("accept_new_client - accept");
	}

	/*** ASK: should we set with different timeout for "handshake" phase? ***/
	// set socket timeout
	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &communication_timeout, sizeof(communication_timeout)) < 0)
		printf("Setsockopt failed for socket id %i\n", socket);
	if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
			(char *) &communication_timeout, sizeof(communication_timeout)) < 0)
		printf("Setsockopt failed for socket id %i\n", socket);

	// wait for info about service provided by the VM
	// if errors occur close the socket
	printf("Waiting for info by client\n");
	if ((numbytes = recv(socket, &service, sizeof(struct vm_service), 0))
			== -1) {
		perror("accept_new_client - recv");
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			perror("accept_new_client - recv timeout");
			close(socket);
		}
	} else if (numbytes == 0) {
		perror("accept_new_client - disconnected VM");
		close(socket);
	} else {
		//control that CN provide an existing service
		while (1) {
			bzero(buffer, BUFSIZE);
			if (service.service >= NUMBER_GROUPS) {
				sprintf(buffer, "groups");
			} else
				sprintf(buffer, "ok");
			if ((send(socket, buffer, BUFSIZE, 0)) == -1) {
				perror("accept_new_client - send");
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					perror("accept_new_client - send timeout");
					fflush(stdout);
				}
				close(socket);
			}
			if (!strcmp(buffer, "ok")) {
				break;
			}
			if ((numbytes = recv(socket, &service, sizeof(struct vm_service), 0))
					== -1) {
				perror("accept_new_client - recv");
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					perror("accept_new_client - recv timeout");
					close(socket);
				}
			} else if (numbytes == 0) {
				perror("accept_new_client - disconnected VM");
				close(socket);
			}
		}

		// increment number of connected CNs
		pthread_mutex_lock(&mutex);

		struct virtual_machine * new_vm = (struct virtual_machine *) malloc(
				sizeof(struct virtual_machine));
		strcpy(new_vm->ip, inet_ntoa(client.sin_addr));
		new_vm->socket = socket;
		new_vm->port = ntohs(client.sin_port);

		if (get_number_of_active_vms(vm_list) < number_of_active_vm) {
			strcpy(vm_op.ip, inet_ntoa(client.sin_addr));
			vm_op.port = htons(8080);
			vm_op.service = service.service;
			vm_op.op = ADD;
			send_command_to_load_balancer();
			new_vm->state = ACTIVE;
		} else {
			new_vm->state = STAND_BY;
		}

		add_vm(new_vm, &vm_list);
		// make a new thread for each VMs
		printf("New VM with IP address %s added with sock id %d, state %i\n", new_vm->ip,
				new_vm->socket, new_vm->state);
		printf("-----------------\nCurrent vm list:\n");
		print_vm_list(vm_list);
		pthread_attr_init(&pthread_custom_attr);
		if (pthread_create(&tid, &pthread_custom_attr, communication_thread,
				(void *) new_vm) != 0) {
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
int accept_load_balancer(int sockfd, pthread_attr_t pthread_custom_attr,
		int * sock_balancer) {
	unsigned int addr_len;
	int socket;
	int numbytes;
	pthread_t tid_balancer_arrival_rate;
	int index;
	int n;

	struct sockaddr_in balancer;
	addr_len = sizeof(struct sockaddr_in);

	// get the first pending VM connection request
	printf("Accepting load_balancer\n");
	if ((socket = accept(sockfd, (struct sockaddr *) &balancer, &addr_len))
			== -1) {
		perror("accept_load_balancer - accept");
		return (int) -1;
	}
	//my_balancer_ip = inet_ntoa(balancer.sin_addr);

	/*** ASK: should we set with different timeout for "handshake" phase? ***/
	// set socket timeout
	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &communication_timeout, sizeof(communication_timeout)) < 0)
		printf("Setsockopt failed for socket id %i\n", socket);
	if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
			(char *) &communication_timeout, sizeof(communication_timeout)) < 0)
		printf("Setsockopt failed for socket id %i\n", socket);
	*sock_balancer = socket;
	// make a new thread for each VMs
	printf(
			"Communication with load_balancer(private ip address) %s established on port %d\n",
			inet_ntoa(balancer.sin_addr), ntohs(balancer.sin_port));
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
void start_server(int * sockfd, int port) {
	int sock;
	struct sockaddr_in temp;

	//Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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

int main(int argc, char ** argv) {
	int sockfd;				//socket number for Computing Nodes (CN)
	int port;				//port number for CN
	int port_balancer;		//port number for LB
	int port_balancer_arrival_rate;
	int index;

	int sock_dgram;

	pthread_attr_t pthread_custom_attr;
	pthread_t tid;
	pthread_t tid_balancer_arrival_rate;
	pthread_t tid_controller_communication;
	communication_timeout.tv_sec = COMMUNICATION_TIMEOUT;
	communication_timeout.tv_usec = 0;

	if (argc != 7) {
		/*** TODO: added argv[0] to avoid warning caused by %s ***/
		printf(
				"Usage: %s vm_port_number load_balancer_port_number LB_arrival_rate_port_number ml_model_number i_am_leader leader_ip\n",
				argv[0]);
		exit(1);
	} else if (atoi(argv[1]) == atoi(argv[2])) {
		printf(
				"Port numbers must be different! Both the ports have the same port number %d\n",
				atoi(argv[1]));
		exit(1);
	}

	port = atoi(argv[1]);
	port_balancer = atoi(argv[2]);
	port_balancer_arrival_rate = atoi(argv[3]);
	ml_model = 1;
	number_of_active_vm=atoi(argv[4]);
	i_am_leader = atoi(argv[5]);
	strcpy(leader_ip, argv[6]);

	//Open the local connection
	start_server(&sockfd, port);
	printf("Server for VMs started, listening on socket %d on port %d\n",
			sockfd, port);
	//Open the connection with the load_balancer
	start_server(&sockfd_balancer, port_balancer);
	printf(
			"Server for load balancer started, listening on socket %d on port %d\n",
			sockfd_balancer, port_balancer);
	start_server(&sockfd_balancer_arrival_rate, port_balancer_arrival_rate);

	//Start dedicated thread to communicate with LB
	//It must block until the system is not ready
	if ((accept_load_balancer(sockfd_balancer, pthread_custom_attr,
			&sockfd_balancer)) < 0)
		exit(1);

	if (sock_read(sockfd_balancer, my_balancer_ip, 16) < 0) {
		perror("Error in reading balancer public ip address: ");
	}
	printf("Balancer correctely connected with public ip address %s\n",
			my_balancer_ip);

	if ((accept_load_balancer(sockfd_balancer_arrival_rate, pthread_custom_attr,
			&sockfd_balancer_arrival_rate)) < 0)
		exit(1);

	pthread_attr_init(&pthread_custom_attr);
	pthread_create(&tid_balancer_arrival_rate, &pthread_custom_attr,
			get_region_features, (void *) (long) sockfd_balancer_arrival_rate);
	//pthread_create(&tid,&pthread_custom_attr,mttf_thread,NULL);

	//start_server_dgram(&sock_dgram);

	socket_controller_communication = socket(AF_INET, SOCK_STREAM, 0);
	if (i_am_leader) {
		memset(regions, 0, NUMBER_REGIONS * sizeof(struct _region));
		get_my_own_ip();
		printf("MY OWN IP IS: %s\n", my_own_ip);
		strcpy(regions[0].ip_controller, my_own_ip);
		strcpy(regions[0].ip_balancer, my_balancer_ip);
		start_server(&socket_controller_communication,
				(int) GLOBAL_CONTROLLER_PORT);
		pthread_create(&tid_controller_communication, &pthread_custom_attr,
				controller_communication_thread, NULL);
	} else {
		struct sockaddr_in controller;
		controller.sin_family = AF_INET;
		controller.sin_addr.s_addr = inet_addr(leader_ip);
		controller.sin_port = htons((int) GLOBAL_CONTROLLER_PORT);
		if (connect(socket_controller_communication,
				(struct sockaddr *) &controller, sizeof(controller)) < 0) {
			perror("Error in connection to leader controller: \n");
		}
		printf(
				"Communication correctely established with leader controller!\n");
	}
	//Init of broadcast and leader primitives
	//initialize_broadcast(PATH);
	//initialize_leader(PATH);

	//Accept new clients
	while (1) {
		accept_new_client(sockfd, pthread_custom_attr);
	}

}

