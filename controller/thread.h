/**
*			Copyright (C) 2008-2015 HPDCS Group
*			http://www.dis.uniroma1.it/~hpdcs
*
*
* This file has been developed by the High Performance and Dependable
* Computing Systems Group.
* It has been released to the public under the GPL license v2.
* Removing this copyright notice is an infringement of current international
* laws.
*
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
*
* This is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this code; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*
* @file thread.h
* @brief This is a module simplify the creation of threads 
* @author Alessandro Pellegrini
*/
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


