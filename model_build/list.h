#pragma once
#ifndef _LIST_H
#define _LIST_H

#define ORDERED		1001
#define UNORDERED	1002

#define ORDER_ASC	2001
#define ORDER_DESC	2002


/* CONFIGURATION OF THE LIST */
#define LIST_ORDER	UNORDERED
#define LIST_ORDER_TYPE	ORDER_ASC // Unused if LIST_ORDER == UNORDERED

typedef struct node *list_link;

struct node {
	double key; // This is used differently: in input, is the elapsed time from VM start, in output is the RTTC
	double generation_time; // This is used to keep track of the time taken to generate two consecutive datapoints by the feature monitor client
	double n_th_slope;
	double mem_used_slope;
	double mem_free_slope;
	double mem_shared_slope;
	double mem_buffers_slope;
	double mem_cached_slope;
	double swap_used_slope;
	double swap_free_slope;
	double cpu_user_slope;
	double cpu_nice_slope;
	double cpu_system_slope;
	double cpu_iowait_slope;
	double cpu_steal_slope;
	double cpu_idle_slope;
	int n_th;
	int mem_used;
	int mem_free;
	int mem_shared;
	int mem_buffers;
	int mem_cached;
	int swap_used;
	int swap_free;
	double cpu_user;
	double cpu_nice;
	double cpu_system;
	double cpu_iowait;
	double cpu_steal;
	double cpu_idle; 

	list_link next;
};

typedef struct {
	list_link head;
	list_link tail;
}list;


extern void initList(list *l);
void insertFront(list *l,list_link temp);
void insertRear(list *l,list_link temp);
void insertNext(list *l,list_link temp,int x);
void deleteNode(list *l,int x);


#endif /* _LIST_H */

