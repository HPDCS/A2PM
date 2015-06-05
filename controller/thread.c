#include <stdlib.h>
#include <stdbool.h>
#include "thread.h"
#include "atomic.h"

static tid_t os_tid;

__thread unsigned int tid;

static unsigned int thread_counter = 0;


/**
* This helper function is the actual entry point for every thread created using the provided internal
* services. The goal of this function is to silently set the new thread's tid so that any place in the
* simulator will find that already set.
* Additionally, when the created thread returns, it frees the memory used to maintain the real entry point
* and the pointer to its arguments.
*
* @author Alessandro Pellegrini
*
* @param arg A pointer to an internally defined structure keeping the real thread's entry point and its arguments
*
* @return This function always returns NULL
*
*/
static void *__helper_create_thread(void *arg) {

	struct _helper_thread *real_arg = (struct _helper_thread *)arg;

	// Get a unique local thread id...
	unsigned int old_counter;

	while(true) {
		old_counter = thread_counter;
		tid = old_counter + 1;
		if(iCAS(&thread_counter, old_counter, tid)) {
			break;
		} 
	}

	// Now get into the real thread's entry point
	real_arg->start_routine(real_arg->arg);

	free(real_arg);

	// We don't really need any return value
	return NULL;
}




/**
* This function creates n threads, all having the same entry point and the same arguments.
* It creates a new thread starting from the __helper_create_thread function which silently
* sets the new thread's tid.
* Note that the arguments passed to __helper_create_thread are malloc'd here, and free'd there.
* This means that if start_routine does not return, there will be a memory leak.
* Additionally, note that arguments pointer by arg are not actually copied here, so all the
* created threads will share them in memory. Changing passed arguments from one of the newly
* created threads will result in all the threads seeing the change.
*
* @author Alessandro Pellegrini
*
* @param start_routine The new threads' entry point
* @param arg A pointer to an array of arguments to be passed to the new threads' entry point
*
*/
int create_threads(unsigned short int n, void *(*start_routine)(void*), void *arg) {
//void create_threads(unsigned short int n, void *(*start_routine)(void*), void *arg) {

	int i;
	int num;
	// We create our thread within our helper function, which accepts just
	// one parameter. We thus have to create one single parameter containing
	// the original pointer and the function to be used as real entry point.
	// This malloc'd array is free'd by the helper function.
	struct _helper_thread *new_arg = malloc(sizeof(struct _helper_thread));
	new_arg->start_routine = start_routine;
	new_arg->arg = arg;

	// n threads are created simply looping...
	for(i = 0; i < n; i++) {
		num = new_thread(__helper_create_thread, (void *)new_arg);
	}
	
	return num;
}

