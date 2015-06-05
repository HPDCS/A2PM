#pragma once

#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>


// How do we identify a thread?
typedef pthread_t tid_t;

/// Spawn a new thread
#define new_thread(entry, arg)	pthread_create(&os_tid, NULL, entry, arg)


/// This structure is used to call the thread creation helper function
struct _helper_thread {
	void *(*start_routine)(void*);
	void *arg;
};


/// Macro to create one single thread
#define create_thread(entry, arg) (create_threads(1, entry, arg))

//void create_threads(unsigned short int n, void *(*start_routine)(void*), void *arg);
int create_threads(unsigned short int n, void *(*start_routine)(void*), void *arg);

extern __thread unsigned int tid;


