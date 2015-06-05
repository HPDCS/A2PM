/*
 * features.h
 *
 *  Created on: Dec 10, 2014
 *      Author: io
 */

#ifndef FEATURES_H_
#define FEATURES_H_

typedef struct _system_features {
	float time;
	int n_th;
	int mem_total;
	int mem_used;
	int mem_free;
	int mem_shared;
	int mem_buffers;
	int mem_cached;
	int swap_total;
	int swap_used;
	int swap_free;
	float cpu_user;
	float cpu_nice;
	float cpu_system;
	float cpu_iowait;
	float cpu_steal;
	float cpu_idle;
} system_features;

typedef struct _system_features_with_slopes {
	float gen_time;
	float n_th_slope;
	float mem_used_slope;
	float mem_free_slope;
	float swap_used_slope;
	float swap_free_slope;
	float cpu_user_slope;
	float cpu_system_slope;
	float cpu_idle_slope;
	int n_th;
	int mem_total;
	int mem_used;
	int mem_free;
	int mem_shared;
	int mem_buffers;
	int mem_cached;
	int swap_total;
	int swap_used;
	int swap_free;
	float cpu_user;
	float cpu_nice;
	float cpu_system;
	float cpu_iowait;
	float cpu_steal;
	float cpu_idle;
} system_features_with_slopes;

#endif /* FEATURES_H_ */
