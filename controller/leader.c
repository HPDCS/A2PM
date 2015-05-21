#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <stdbool.h>
#include <math.h>

#include "sockhelp.h"
#include "broadcast.h"
#include "thread.h"
#include "timer.h"

#define PATH "/home/luca/Scrivania/controllers_list.txt"

#define LEADER_SLEEP 1
#define LEADER_SUSPECT_THRESHOLD 10
#define LEADER_PROPOSE_THRESHOLD 10


#define LEADER_HEARTBEAT	20000
#define LEADER_PROPOSE		20001

static unsigned int controllers;

static int leader_socket = -1;
static bool am_i_leader = false;

timer heartbeat;
timer leader_propose_timer;

static bool agreement_running = false;

static long leader_proposals[MAX_CONTROLLERS];
static int leader_proposals_sockets[MAX_CONTROLLERS];
static int last_proposal;


static void do_agreement_reduction(void) {
	int i;
	int leader = -1;
	int id = 0;

/*printf("LEADER: last_proposal: %d\n", last_proposal);
printf("LEADER: controllers: %d\n", controllers);*/

	if(last_proposal < controllers && timer_value_seconds(leader_propose_timer) <= LEADER_PROPOSE_THRESHOLD)
		return;
//printf("LEADER: ho superato l'if\n");
	for(i = 0; i < last_proposal; i++) {
		if(leader_proposals[i] > id) {
			id = leader_proposals[i];
			leader = leader_proposals_sockets[i];
		}
	}

	if(leader == -1) {
		fprintf(stderr, "%s:%d: Error in leader election\n", __FILE__, __LINE__);
		agreement_running = false;
		return;
	}

	agreement_running = false;
	leader_socket = leader;
}


static void do_leader_election(void) {

	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	long my_int_ip;

	if(agreement_running) {
		do_agreement_reduction();
		return;
	}

	agreement_running = true;
	last_proposal = 0;
	timer_start(leader_propose_timer);

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}


	/* Walk through linked list, maintaining head pointer so we
	can free list later */
	

/***************/
/*	
	char host[NI_MAXHOST];
	
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
	   family = ifa->ifa_addr->sa_family;

	   printf("%s  address family: %d%s\n",
			   ifa->ifa_name, family,
			   (family == AF_PACKET) ? " (AF_PACKET)" :
			   (family == AF_INET) ?   " (AF_INET)" :
			   (family == AF_INET6) ?  " (AF_INET6)" : "");

	   if (family == AF_INET || family == AF_INET6) {
		   s = getnameinfo(ifa->ifa_addr,
				   (family == AF_INET) ? sizeof(struct sockaddr_in) :
										 sizeof(struct sockaddr_in6),
				   host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		   if (s != 0) {
			   printf("getnameinfo() failed: %s\n", gai_strerror(s));
			   exit(EXIT_FAILURE);
		   }
		   printf("\taddress: <%s>\n", host);
	   }
   }
*/
/*********************/
	

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET && !strcmp(ifa->ifa_name,"eth0")) {
			my_int_ip = (long)(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr);
			printf("MY_INT_IP: %ld\n", my_int_ip);
			printf("Proposed ip_address: %s on net interface %s\n", inet_ntoa(((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr), ifa->ifa_name);
			broadcast(LEADER_PROPOSE, &my_int_ip, sizeof(my_int_ip));
			goto ip_found;

		}
	}
	
	fprintf(stderr, "Error: unable to get my own IP. I'm not participating in the leader election!\n");

    ip_found:

	freeifaddrs(ifaddr);
}


static void leader_propose(int sock, void *content, size_t size) {
	(void)size;

	leader_proposals_sockets[last_proposal] = sock;
	leader_proposals[last_proposal++] = *(long *)content;
}


static void leader_heartbeat(int sock, void *content, size_t size) {
	(void)content;
	(void)size;

	timer_restart(heartbeat);
}

	
static void suspect_leader(void) {
	if(timer_value_seconds(heartbeat) > LEADER_SUSPECT_THRESHOLD) {

		close(leader_socket);
		leader_socket = -1;
		if(am_i_leader) // Stilly sanity check
			am_i_leader = false;
	}
}


static void *leader_loop(void *args) {
	(void)args;

	while(true) {
		sleep(LEADER_SLEEP);

		suspect_leader();

		if(leader_socket == -1) {
			do_leader_election();
		}

		if(am_i_leader) {
			broadcast(LEADER_HEARTBEAT, NULL, 0);
		}
	}
}



bool send_to_leader(void *payload, size_t size) {
	if(leader_socket == -1)
		return false;

	if(sock_write(leader_socket, payload, size) < size)
		return false;

	return true;
}


int initialize_leader(char *controllers_path) {

	printf("Initializing leader election...\n");

	// Count controllers
	FILE *f;
        char line[128];
        int i = 0;

        f = fopen(controllers_path, "r");
        if(f == NULL) {
                perror("Unable to count controllers for leader election");
                exit(EXIT_FAILURE);
        }

        while (fgets(line, 128, f) != NULL) {
			controllers++;
        }

	

	timer_start(heartbeat);
	register_callback(LEADER_HEARTBEAT, leader_heartbeat);
	register_callback(LEADER_PROPOSE, leader_propose);
	create_thread(leader_loop, NULL);
	
	printf("Leader election UP!\n");
}
