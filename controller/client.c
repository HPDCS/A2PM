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
#include <unistd.h>

#define BUFSIZE 								4096
#define TIME_FOR_COMPLETING_PENDING_REQUESTS	50

// services provided
enum services{
	SERVICE_0, SERVICE_1, SERVICE_2
};

// STAND_BY has value 0, ACTIVE has value 1, REJUVENATING has value 2, and so on...
enum vm_state {
    STAND_BY, ACTIVE, REJUVENATING
};

// service info for each VM
struct vm_service{
    enum services service;
    enum vm_state state;
};

struct vm_service s;

struct timeval initial_time;
int num_cpus;
time_t now;

char send_buff[BUFSIZE];
char recv_buff[BUFSIZE];

// Get the features from the feature monitor client
void get_features(char * output){
    FILE * t;
    
    int index; //used to fill the aux_buffer
    char aux_buffer[BUFSIZE]; //used to get /proc/meminfo file content
    char * pointer_buffer; //used to retrieve inside the buffer
    char ch; //used to fill the aus_buffer
    
    int mem_total, mem_used, mem_free, mem_shared, mem_buffers, mem_cached;
    int swap_total, swap_used, swap_free;
    unsigned long cpu_user1, cpu_nice1, cpu_system1, cpu_iowait1, cpu_steal1, cpu_idle1, dummyA1, dummyB1, dummyC1, dummyD1;
    unsigned long cpu_user2, cpu_nice2, cpu_system2, cpu_iowait2, cpu_steal2, cpu_idle2, dummyA2, dummyB2, dummyC2, dummyD2;
    float cpu_user, cpu_nice, cpu_system, cpu_iowait, cpu_steal, cpu_idle;
    float cpu_total;
    struct timeval curr_time;
    
    char num_th[128];
    FILE *pof, *fstat, *fmem;
    
    /* Get number of active threads */
    pof = popen("ps -eLf | grep -v defunct | wc -l", "r");
    if(pof == NULL)
        abort();
    
    fgets(num_th, sizeof(num_th)-1, pof);
    pclose(pof);
    
    /* Get timestamp */
    gettimeofday(&curr_time, NULL);
    sprintf(output, "Datapoint: %f %s", (double)curr_time.tv_sec - initial_time.tv_sec + (double)curr_time.tv_usec / 1000000 - (double)initial_time.tv_usec / 1000000, num_th);
    
    /* Get memory and swap usage (using /proc/meminfo) */
    t = fopen("/proc/meminfo", "r");
    if (t == NULL) {
        perror("FOPEN ERROR MEMINFO ");
        exit(EXIT_FAILURE);
    }
    index = 0;
    bzero(aux_buffer,BUFSIZE);
    while ((ch = fgetc(t)) != EOF) {
        aux_buffer[index++] = ch;
    }
    fclose(t);
    sscanf(aux_buffer,"MemTotal: %d kB MemFree: %d kB Buffers: %d kB Cached: %d kB", &mem_total, &mem_free, &mem_buffers, &mem_cached);
    mem_used = mem_total - mem_free;
    mem_shared = 0; /*** ASK: where can i get this info? ***/
    sprintf(output, "%sMemory: %d %d %d %d %d %d\n", output, mem_total, mem_used, mem_free, mem_shared, mem_buffers, mem_cached);
    pointer_buffer = strstr(aux_buffer,"SwapTotal:");
    sscanf(pointer_buffer,"SwapTotal: %d kB SwapFree: %d kB", &swap_total, &swap_free);
    swap_used = swap_total - swap_free;
    sprintf(output, "%sSwap: %d %d %d\n", output, swap_total, swap_used, swap_free);
    
    /* Get CPU Usage (using /proc/stat) */
    fstat = fopen("/proc/stat", "r");
    if (fstat == NULL) {
        perror("FOPEN ERROR FIRST /PROC/STAT ");
        exit(EXIT_FAILURE);
    }
    if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_user1, &cpu_nice1, &cpu_system1, &cpu_idle1, &cpu_iowait1, &dummyA1, &dummyB1, &cpu_steal1, &dummyC1, &dummyD1) == EOF) {
        exit(EXIT_FAILURE);
    }
    fclose(fstat);
    sleep(1);
    fstat = fopen("/proc/stat", "r");
    if (fstat == NULL) {
        perror("FOPEN ERROR SECOND /PROC/STAT ");
        exit(EXIT_FAILURE);
    }
    if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &cpu_user2, &cpu_nice2, &cpu_system2, &cpu_idle2, &cpu_iowait2, &dummyA2, &dummyB2, &cpu_steal2, &dummyC2, &dummyD2) == EOF) {
        exit(EXIT_FAILURE);
    }
    fclose(fstat);
    
    cpu_total = (float)(cpu_user2 + cpu_nice2 + cpu_system2 + cpu_idle2 + cpu_iowait2 + dummyA2 + dummyB2 + cpu_steal2 + dummyC2 + dummyD2);
    cpu_total -= (float)(cpu_user1 + cpu_nice1 + cpu_system1 + cpu_idle1 + cpu_iowait1 + dummyA1 + dummyB1 + cpu_steal1 + dummyC1 + dummyD1);
    
    cpu_user = (float)(cpu_user2 - cpu_user1) * 100.0 / cpu_total;
    cpu_system = (float)(cpu_system2 - cpu_system1) * 100.0 / cpu_total;
    cpu_nice = (float)(cpu_nice2 - cpu_nice1) * 100.0 / cpu_total;
    cpu_idle = (float)(cpu_idle2 - cpu_idle1) * 100.0 / cpu_total;
    cpu_iowait = (float)(cpu_iowait2 - cpu_iowait1) * 100.0 / cpu_total;
    cpu_steal = (float)(cpu_steal2 - cpu_steal1) * 100.0 / cpu_total;
    
    sprintf(output,"%sCPU: %f %f %f %f %f %f", output, cpu_user, cpu_nice, cpu_system, cpu_iowait, cpu_steal, cpu_idle);
    
    //printf("%s\n",output);
    
    return;

}

void connect_to_server(int * sockfd, char * server_addr, int port){
    
    struct sockaddr_in server;
    
    printf("Creating socket...\n");
    if((*sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("connect_to_server - socket");
        exit(1);
    }
    printf("Socket created\n");
    
    server.sin_addr.s_addr = inet_addr(server_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    printf("Connecting to the server...\n");
    if (connect(*sockfd, (struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("connect_to_server - connect");
        exit(1);
    }
    // send info about the service to the server
    printf("Connected to the server\nSending service info to the server...\n");
    if ((send(*sockfd, &s, sizeof(struct vm_service), 0)) == -1) {
        perror("connect_to_server - send");
        close(*sockfd);
        exit(1);
    }
    printf("Waiting for server ack...\n");
    while(1){
		bzero(recv_buff,BUFSIZE);
		
		if ((recv(*sockfd, recv_buff, BUFSIZE,0)) == -1){
			perror("connect_to_server - recv");
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				perror("connect_to_server - recv timeout");
				fflush(stdout);
			}
			close(*sockfd);
		}
		if (!strcmp(recv_buff,"ok")){
			break;
		}
		printf("Insert a correct value for the service: ");
		scanf("%u", &s.service);
		if ((send(*sockfd, &s, sizeof(struct vm_service), 0)) == -1) {
			perror("connect_to_server - send");
			close(*sockfd);
			exit(1);
		}
	}
}

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
    if (listen(sock, 1024)) {
        perror("start_server - listen");
        exit(1);
    }
    *sockfd = sock;
    printf("Server started\n");
    
}

int main(int argc,char ** argv){
    int sockfd;
    int numbytes;
    int group;
    int status;
    pthread_attr_t pthread_custom_attr;
    pthread_t tid;
   
    if (argc != 5) {
		// server stays for controller server side
        printf("Usage: %s server_ip_address server_tcp_port_number service status\n", argv[0]);
        exit(1);
    } 
       
    // store info about the service provided
    s.service = atoi(argv[3]);
    if(!strcmp(argv[4],"STAND_BY"))
		s.state = STAND_BY;
    else if (!strcmp(argv[4],"ACTIVE"))
		s.state = ACTIVE;
	else{
		char buff[4096];
		do{
			printf("Invalid status! ACTIVE or STAND_BY? : ");
			scanf("%s", buff);
			if(!strcmp(buff,"STAND_BY")){
				s.state = STAND_BY;
				break;
			}
			else if (!strcmp(buff,"ACTIVE")){
				s.state = ACTIVE;
				break;
			}
		}while(1);
		
    }
    connect_to_server(&sockfd, argv[1], atoi(argv[2]));

    gettimeofday(&initial_time, NULL);
    
    //get number of available CPUs
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    while (1) {
//        time(&now);
//        printf("Time: %s", ctime(&now));
        printf("Collecting data features and sending to server...\n");
        bzero(send_buff, BUFSIZE);
        get_features(send_buff);
        if ((numbytes = send(sockfd, send_buff, BUFSIZE, 0)) == -1) {
            perror("main - send");
            break;
        }
        
        printf("%d bytes sent\n",numbytes);
        printf("Waiting for server commands...\n");
        fflush(stdout);
        bzero(recv_buff, BUFSIZE);
        if (recv(sockfd, recv_buff, BUFSIZE, 0) == -1) {
            perror("main - recv");
            break;
        }
        
        
        if (recv_buff[0]==REJUVENATE) {
            //wait for completing pending requests
            printf("Command REJUVENATE received\n");
            printf("Waiting %d seconds for completing pending requests before rejuvenation...\n", TIME_FOR_COMPLETING_PENDING_REQUESTS);
            fflush(stdout);
            sleep(TIME_FOR_COMPLETING_PENDING_REQUESTS);
            printf("Executing reboot...\n");
            fflush(stdout);
            system("reboot");
            exit(0);
        }

	sleep(5);
        
    }
    
    close(sockfd);
    
    return 0;
}
