#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>


#define KRED  "\x1B[31m"
#define RESET "\033[0m"


#define MIN_MEAN 50000000	// in usec (50 seconds)
#define MAX_MEAN 200000000	// in usec (200 seconds)


#define CLOCK_READ() ({ \
			unsigned int lo; \
			unsigned int hi; \
			__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi)); \
			((unsigned long long)hi) << 32 | lo; \
			})


#define Random() ((double)random() / (RAND_MAX - 1))

double Expent(double mean) {
	
	if(mean < 0) {
		fprintf(stderr, "Error: in call to Expent() passed a negative mean value\n");
		abort();
	}

	return (-mean * log(1 - Random()));
}



int RandomRange(int min, int max) {
	return (int)floor(Random() * (max - min + 1)) + min;
}


void *steal_my_cpu(void *arg) {

	char *str = "abcdefghijklm";
	char *ptr;
	unsigned int i;

	// Aha!
	for(;;) {

		for(i = 0; i < 1000; i++) {
			ptr = str;
			while(*ptr != '\0') {
				*ptr++;
				ptr++;
			}
		}


		if(RandomRange(1,10) < 0.01) {
			usleep(10000 * RandomRange(1,10));
		}
	}

	return NULL;
}

int main(void) {
    double current_mean;
	double waiting_time;
	int count = 0;
	int err;
	double one_wait_time;

	pthread_t tid;

	srandom(CLOCK_READ());
	printf("Starting to create threads!\n");

	while(1) {
		current_mean = RandomRange(MIN_MEAN, MAX_MEAN);
		one_wait_time = Expent(current_mean);
		waiting_time = 0;
rewait:
		waiting_time += one_wait_time;
		if(RandomRange(MIN_MEAN, MAX_MEAN) < ((double)(MIN_MEAN + MAX_MEAN) / 5.0))
			goto rewait;
		if(RandomRange(MIN_MEAN, MAX_MEAN) < ((double)(MIN_MEAN + MAX_MEAN) / 50.0))
			waiting_time *= 40;
		printf("Next thread in %f seconds... ", (float)waiting_time/1000000);
		fflush(stdout);
		usleep(waiting_time);
        err = pthread_create(&tid, NULL, steal_my_cpu, NULL);
        if (err != 0)
            printf("can't create thread: %s\n", strerror(err));
        else
            printf(KRED "THREADS" RESET ": thread %d created successfully\n", ++count);
	}

	return 0;
}
