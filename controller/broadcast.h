/**
 * This is the broadcast header, to send messages to everyone
 */

#pragma once

#include <stdlib.h>
#include "sockhelp.h"

#define MAX_CONTROLLERS 32
#define MAX_CALLBACKS 	1024
//#define MAX_MESSAGE 	1024*1024
#define MAX_MESSAGE		1024
#define SIZE_PAYLOAD	1024

typedef struct _msg {
	int type;
	size_t size;
	//void * payload;
	long payload;
} msg_struct;

typedef struct _cb {
	int type;
	//void (*callback)(int sock, void *content, size_t size);
	void (*callback)(int sock, long content, size_t size);
} callback_struct;

void initialize_broadcast(const char *controllers_path);
void broadcast(int type, long payload, size_t data);
//void broadcast(int type, void *payload, size_t data);
//void register_callback(int type, void (*f)(int sock, void *content, size_t size));
void register_callback(int type, void (*f)(int sock, long content, size_t size));
