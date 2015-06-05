#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

#include "list.h"

//#define VERBOSE

#define MAX_RUNS 1024
#define AGGREGATION_STEP 15 // in seconds
#define MAX_LINE_LENGTH 1024

char *out_file_name;
FILE *in_file;
FILE *out_file;

list *input_datapoints[MAX_RUNS];
list *aggregated_datapoints;

unsigned int curr_run = 0;


void read_db_file(void) {
	list *curr_list = NULL;
	char line[MAX_LINE_LENGTH];
	struct node a_new_point;
	char *token;
	bool datapoint_fully_filled = false;
	unsigned int read_nodes = 0;
	unsigned int line_count = 0;
	
	printf("Starting to parse input database...\n");
	#ifdef VERBOSE
	sleep(1);
	#endif

	while(fgets(line, MAX_LINE_LENGTH, in_file) != 0) {
		
		line_count++;
				
		#ifdef VERBOSE
		printf("%s", line);
		#endif
		
		// Getting data from a new run from now on
		if(strcmp(line, "------------NEW ACTIVE MACHINE:----------\n") == 0) {
			input_datapoints[curr_run] = malloc(sizeof(list));
			initList(input_datapoints[curr_run]);
			curr_list = input_datapoints[curr_run++];
			continue;
		}
		
		// Fill the data from the next datapoint
		token = strtok(line, ":");
		if(strcmp(token, "Datapoint") == 0) {
			
			// Datapoint format contains the time and the number of active threads
			sscanf(strtok(NULL, ":"), "%lf %d\n", &a_new_point.key,
							     &a_new_point.n_th);
			
		} else if(strcmp(token, "Memory") == 0) {
			
			// Memory point is in the form:
			// mem_used mem_free mem_shared mem_buffers mem_cached
			sscanf(strtok(NULL, ":"), "%d %d %d %d %d %d\n", &a_new_point.mem_used, // First one is 'mem_total', which is of no interest
									 &a_new_point.mem_used,
									 &a_new_point.mem_free,
									 &a_new_point.mem_shared,
									 &a_new_point.mem_buffers,
									 &a_new_point.mem_cached);
			
		} else if(strcmp(token, "Swap") == 0) {
			
			sscanf(strtok(NULL, ":"), "%d %d %d\n", &a_new_point.swap_used, // First one is 'swap_total', which is of no interest
								&a_new_point.swap_used,
								&a_new_point.swap_free);
			
		} else if(strcmp(token, "CPU") == 0) {
			
			sscanf(strtok(NULL, ":"), "%lf %lf %lf %lf %lf %lf\n", &a_new_point.cpu_user,
									       &a_new_point.cpu_nice,
									       &a_new_point.cpu_system,
									       &a_new_point.cpu_iowait,
									       &a_new_point.cpu_steal,
									       &a_new_point.cpu_idle);
			// This is the last entry of a datapoint
			datapoint_fully_filled = true;
			
		} else {
			fprintf(stderr, "Unknown token: '%s' at line %d\n", token, line_count);
			exit(EXIT_FAILURE);
		}



		// Have we completed a new datapoint?
		if(datapoint_fully_filled) {
			
			// This fails if no NEW ACTIVE MACHINE message is found at beginning of ile
			assert(curr_list != NULL);
			
			struct node *new_node = malloc(sizeof(struct node));
			memcpy(new_node, &a_new_point, sizeof(struct node));
			insertRear(curr_list, new_node);
			
			#ifdef VERBOSE
			printf("\tInserted new datapoint in run %d with content:\n"
			       "\t\tTime: %f\n"
			       "\t\tn_th: %d\n"
			       "\t\tmem_used: %d\n"
			       "\t\tmem_free: %d\n"
			       "\t\tmem_shared: %d\n"
			       "\t\tmem_buffers: %d\n"
			       "\t\tmem_cached: %d\n"
			       "\t\tswap_used: %d\n"
			       "\t\tswap_free: %d\n"
			       "\t\tcpu_user: %f\n"
			       "\t\tcpu_nice: %f\n"
			       "\t\tcpu_system: %f\n"
			       "\t\tcpu_iowait: %f\n"
			       "\t\tcpu_steal: %f\n"
			       "\t\t:cpu_idle: %f\n",
			       curr_run,
			       new_node->key,
			       new_node->n_th,
			       new_node->mem_used,
			       new_node->mem_free,
			       new_node->mem_shared,
			       new_node->mem_buffers,
			       new_node->mem_cached,
			       new_node->swap_used,
			       new_node->swap_free,
			       new_node->cpu_user,
			       new_node->cpu_nice,
			       new_node->cpu_system,
			       new_node->cpu_iowait,
			       new_node->cpu_steal,
			       new_node->cpu_idle);
			fflush(stdout);
			#endif
			
			read_nodes++;
			
			bzero(&a_new_point, sizeof(struct node));
			datapoint_fully_filled = false;
		}
	}
	
	printf("Parsed %d run%s from database file with %d total datapoints.\n", curr_run, (curr_run > 1 ? "s" : ""), read_nodes);
	#ifdef VERBOSE
	sleep(1);
	#endif

	
}

void aggregate(void) {
	unsigned int curr_list;
	unsigned int num_nodes;
	struct node aggregated;
	struct node *curr_node;
	double init_time = 0.0;
	bool compute_RTTC;
	unsigned int total_aggregated_nodes = 0;
	
	printf("Starting datapoint aggregation with step %d seconds...\n", AGGREGATION_STEP);
	
	for(curr_list = 0; curr_list < curr_run; curr_list++) {
		
		num_nodes = 0;
		compute_RTTC = true;
		bzero(&aggregated, sizeof(struct node));
		curr_node = input_datapoints[curr_list]->head;

		init_time = input_datapoints[curr_list]->head->key;
		
		while(curr_node != NULL) {
			
			if(fabs(curr_node->key - init_time) > AGGREGATION_STEP || curr_node == input_datapoints[curr_list]->tail) {
			
				#ifdef VERBOSE
				printf("INIT TIME is %f, current is %f, step is %d: new aggregated point generated\n", init_time, curr_node->key, AGGREGATION_STEP);
				#endif
				
				// Compute the average generation time
				aggregated.generation_time = (curr_node->key - aggregated.generation_time) / num_nodes;
				
				// Compute average values
				aggregated.n_th /= num_nodes;		
				aggregated.mem_used /= num_nodes;
				aggregated.mem_free /= num_nodes;
				aggregated.mem_shared /= num_nodes;
				aggregated.mem_buffers /= num_nodes;
				aggregated.mem_cached /= num_nodes;
				aggregated.swap_used /= num_nodes;
				aggregated.swap_free /= num_nodes;
				aggregated.cpu_user /= num_nodes;
				aggregated.cpu_nice /= num_nodes;
				aggregated.cpu_system /= num_nodes;
				aggregated.cpu_iowait /= num_nodes;
				aggregated.cpu_steal /= num_nodes;
				aggregated.cpu_idle /= num_nodes;
				
				// Finish to compute the slopes
				aggregated.n_th_slope -= curr_node->n_th;
				aggregated.mem_used_slope -= curr_node->mem_used;
				aggregated.mem_free_slope -= curr_node->mem_free;
				aggregated.mem_shared_slope -= curr_node->mem_shared;
				aggregated.mem_buffers_slope -= curr_node->mem_buffers;
				aggregated.mem_cached_slope -= curr_node->mem_cached;
				aggregated.swap_used_slope -= curr_node->swap_used;
				aggregated.swap_free_slope -= curr_node->swap_free;
				aggregated.cpu_user_slope -= curr_node->cpu_user;
				aggregated.cpu_nice_slope -= curr_node->cpu_nice;
				aggregated.cpu_system_slope -= curr_node->cpu_system;
				aggregated.cpu_iowait_slope -= curr_node->cpu_iowait;
				aggregated.cpu_steal_slope -= curr_node->cpu_steal;
				aggregated.cpu_idle_slope -= curr_node->cpu_idle;
				
				aggregated.n_th_slope /= -1.0 * num_nodes;
				aggregated.mem_used_slope /= -1.0 * num_nodes;
				aggregated.mem_free_slope /= -1.0 * num_nodes;
				aggregated.mem_shared_slope /= -1.0 * num_nodes;
				aggregated.mem_buffers_slope /= -1.0 * num_nodes;
				aggregated.mem_cached_slope /= -1.0 * num_nodes;
				aggregated.swap_used_slope /= -1.0 * num_nodes;
				aggregated.swap_free_slope /= -1.0 * num_nodes;
				aggregated.cpu_user_slope /= -1.0 * num_nodes;
				aggregated.cpu_nice_slope /= -1.0 * num_nodes;
				aggregated.cpu_system_slope /= -1.0 * num_nodes;
				aggregated.cpu_iowait_slope /= -1.0 * num_nodes;
				aggregated.cpu_steal_slope /= -1.0 * num_nodes;
				aggregated.cpu_idle_slope /= -1.0 * num_nodes;
				
				struct node *new_node = malloc(sizeof(struct node));
				memcpy(new_node, &aggregated, sizeof(struct node));
				insertRear(aggregated_datapoints, new_node);

				total_aggregated_nodes++;
				init_time = curr_node->key;
				num_nodes = 0;
				compute_RTTC = true;
				
				#ifdef VERBOSE
				printf("\tAggregated node %d generated with content:\n"
				       "\t\tTime: %f\n"
				       
				       "\t\tn_th_slope: %f\n"
				       "\t\tmem_used_slope: %f\n"
				       "\t\tmem_free_slope: %f\n"
				       "\t\tmem_shared_slope: %f\n"
				       "\t\tmem_buffers_slope: %f\n"
				       "\t\tmem_cached_slope: %f\n"
				       "\t\tswap_used_slope: %f\n"
				       "\t\tswap_free_slope: %f\n"
				       "\t\tcpu_user_slope: %f\n"
				       "\t\tcpu_nice_slope: %f\n"
				       "\t\tcpu_system_slope: %f\n"
				       "\t\tcpu_iowait_slope: %f\n"
				       "\t\tcpu_steal_slope: %f\n"
				       "\t\tcpu_idle_slope: %f\n"
				       
				       "\t\tn_th: %d\n"
				       "\t\tmem_used: %d\n"
				       "\t\tmem_free: %d\n"
				       "\t\tmem_shared: %d\n"
				       "\t\tmem_buffers: %d\n"
				       "\t\tmem_cached: %d\n"
				       "\t\tswap_used: %d\n"
				       "\t\tswap_free: %d\n"
				       "\t\tcpu_user: %f\n"
				       "\t\tcpu_nice: %f\n"
				       "\t\tcpu_system: %f\n"
				       "\t\tcpu_iowait: %f\n"
				       "\t\tcpu_steal: %f\n"
				       "\t\t:cpu_idle: %f\n",
				       total_aggregated_nodes,
				       new_node->key,
       				
				       new_node->n_th_slope,
				       new_node->mem_used_slope,
				       new_node->mem_free_slope,
				       new_node->mem_shared_slope,
				       new_node->mem_buffers_slope,
				       new_node->mem_cached_slope,
				       new_node->swap_used_slope,
				       new_node->swap_free_slope,
				       new_node->cpu_user_slope,
				       new_node->cpu_nice_slope,
				       new_node->cpu_system_slope,
				       new_node->cpu_iowait_slope,
				       new_node->cpu_steal_slope,
				       new_node->cpu_idle_slope,

				       new_node->n_th,
				       new_node->mem_used,
				       new_node->mem_free,
				       new_node->mem_shared,
				       new_node->mem_buffers,
				       new_node->mem_cached,
				       new_node->swap_used,
				       new_node->swap_free,
				       new_node->cpu_user,
				       new_node->cpu_nice,
				       new_node->cpu_system,
				       new_node->cpu_iowait,
				       new_node->cpu_steal,
				       new_node->cpu_idle);
				fflush(stdout);
				#endif
				
				bzero(&aggregated, sizeof(struct node));
			}
			
			// This computes the RTTC and initializes the slopes
			if(compute_RTTC) {

				#ifdef VERBOSE
				printf("(%d) RTTC: %f - %f = %f\n", curr_list, input_datapoints[curr_list]->tail->key, curr_node->key, input_datapoints[curr_list]->tail->key - curr_node->key);
				#endif

				aggregated.key = input_datapoints[curr_list]->tail->key - curr_node->key;
				
				aggregated.generation_time = curr_node->key;
				
				aggregated.n_th_slope = curr_node->n_th;
				aggregated.mem_used_slope = curr_node->mem_used;
				aggregated.mem_free_slope = curr_node->mem_free;
				aggregated.mem_shared_slope = curr_node->mem_shared;
				aggregated.mem_buffers_slope = curr_node->mem_buffers;
				aggregated.mem_cached_slope = curr_node->mem_cached;
				aggregated.swap_used_slope = curr_node->swap_used;
				aggregated.swap_free_slope = curr_node->swap_free;
				aggregated.cpu_user_slope = curr_node->cpu_user;
				aggregated.cpu_nice_slope = curr_node->cpu_nice;
				aggregated.cpu_system_slope = curr_node->cpu_system;
				aggregated.cpu_iowait_slope = curr_node->cpu_iowait;
				aggregated.cpu_steal_slope = curr_node->cpu_steal;
				aggregated.cpu_idle_slope = curr_node->cpu_idle;
				
				compute_RTTC = false;
			}
			
			aggregated.n_th += curr_node->n_th;
			aggregated.mem_used += curr_node->mem_used;
			aggregated.mem_free += curr_node->mem_free;
			aggregated.mem_shared += curr_node->mem_shared;
			aggregated.mem_buffers += curr_node->mem_buffers;
			aggregated.mem_cached += curr_node->mem_cached;
			aggregated.swap_used += curr_node->swap_used;
			aggregated.swap_free += curr_node->swap_free;
			aggregated.cpu_user += curr_node->cpu_user;
			aggregated.cpu_nice += curr_node->cpu_nice;
			aggregated.cpu_system += curr_node->cpu_system;
			aggregated.cpu_iowait += curr_node->cpu_iowait;
			aggregated.cpu_steal += curr_node->cpu_steal;
			aggregated.cpu_idle += curr_node->cpu_idle;
					
			num_nodes++;
			
			curr_node = curr_node->next;
		}
	}
	
	printf("Generated %d aggregated nodes.\n", total_aggregated_nodes);
	
}

void write_aggregated_file(void) {
	struct node *curr_node;
	
	
	printf("Writing aggregated nodes to %s...\n", out_file_name);
	
	fprintf(out_file, "RTTC, "
			  "GenTime, "
			  "n_th_slope, "
			  "mem_used_slope, "
			  "mem_free_slope, "
			  "mem_shared_slope, "
			  "mem_buffers_slope, "
			  "mem_cached_slope, "
			  "swap_used_slope, "
			  "swap_free_slope, "
			  "cpu_user_slope, "
			  "cpu_nice_slope, "
			  "cpu_system_slope, "
			  "cpu_iowait_slope, "
			  "cpu_steal_slope, "
			  "cpu_idle_slope, "
			  "n_th, "
			  "mem_used, "
			  "mem_free, "
			  "mem_shared, "
			  "mem_buffers, "
			  "mem_cached, "
			  "swap_used, "
			  "swap_free, "
			  "cpu_user, "
			  "cpu_nice, "
			  "cpu_system, "
			  "cpu_iowait, "
			  "cpu_steal, "
			  "cpu_idle\n");
	
	curr_node = aggregated_datapoints->head;
	while(curr_node != NULL) {
		
		fprintf(out_file, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, "
				  "%d, %d, %d, %d, %d, %d, %d, %d, %f, %f, %f, %f, %f, %f\n",
  				       curr_node->key,
  				       
  				       curr_node->generation_time,
  				       
  				       curr_node->n_th_slope,
				       curr_node->mem_used_slope,
				       curr_node->mem_free_slope,
				       curr_node->mem_shared_slope,
				       curr_node->mem_buffers_slope,
				       curr_node->mem_cached_slope,
				       curr_node->swap_used_slope,
				       curr_node->swap_free_slope,
				       curr_node->cpu_user_slope,
				       curr_node->cpu_nice_slope,
				       curr_node->cpu_system_slope,
				       curr_node->cpu_iowait_slope,
				       curr_node->cpu_steal_slope,
				       curr_node->cpu_idle_slope,

				       curr_node->n_th,
				       curr_node->mem_used,
				       curr_node->mem_free,
				       curr_node->mem_shared,
				       curr_node->mem_buffers,
				       curr_node->mem_cached,
				       curr_node->swap_used,
				       curr_node->swap_free,
				       curr_node->cpu_user,
				       curr_node->cpu_nice,
				       curr_node->cpu_system,
				       curr_node->cpu_iowait,
				       curr_node->cpu_steal,
				       curr_node->cpu_idle);
		
		curr_node = curr_node->next;
	}
	
	printf("Done.\n");
}


int main(int argc, char **argv) {
	
	// Check on arguments	
	if(argc < 2) {
		fprintf(stderr, "Usage: %s db-input [aggregate-output]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if(argc == 3) {
		out_file_name = argv[2];
	} else {
		out_file_name = "aggregated.csv";
	}
	
	// Open files
	if((in_file = fopen(argv[1], "r")) == NULL) {
		perror("Error opening database file");
		exit(EXIT_FAILURE);
	}
	if((out_file = fopen(out_file_name, "w")) == NULL) {
		perror("Error opening aggregated file for writing results");
		exit(EXIT_FAILURE);
	}

	// Allocate the intermediate list for aggregated datapoints
	aggregated_datapoints = (list*)malloc(sizeof(list));
	initList(aggregated_datapoints);
	
	// Read the input database
	read_db_file();
	
	// Perform aggregation
	aggregate();
	
	// Write back to aggregated file
	write_aggregated_file();

	// Close files
	fclose(in_file);
	fclose(out_file);

	return 0;
}
