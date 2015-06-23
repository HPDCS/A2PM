#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>


// If this line is uncommented, the program prints on screen
// cumulated time and amount of leaks, e.g. to see what is
// the memory wastage trend
//#define SIMULATE

#define KRED  "\x1B[31m"
#define RESET "\033[0m"


#define MIN_LEAK 1024		// in bytes
#define MAX_LEAK 1024*1024	// in bytes

#define MIN_MEAN 500	// in usec (0.5 seconds)
#define MAX_MEAN 1000000	// in usec (2 seconds)


#define CLOCK_READ() ({ \
			unsigned int lo; \
			unsigned int hi; \
			__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi)); \
			((unsigned long long)hi) << 32 | lo; \
			})


#define Random() ((double)random() / (RAND_MAX - 1))



#ifdef SIMULATE
double cumulated_time = 0.0;
unsigned long long cumulated_leak = 0;
#endif



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



int main(void) {
	double current_mean;
	size_t leak;
	double waiting_time;
	double one_wait_time;
	
	srandom(CLOCK_READ());
	
	printf("Running with mean %f\n", current_mean);
	
	while(1) {
		current_mean = RandomRange(MIN_MEAN, MAX_MEAN);
		leak = RandomRange(MIN_LEAK, MAX_LEAK);
		one_wait_time = Expent(current_mean);
		waiting_time = 0;
rewait:
		waiting_time += one_wait_time;
		if(RandomRange(MIN_MEAN, MAX_MEAN) < ((double)(MIN_MEAN + MAX_MEAN) / 5.0))
			goto rewait;
		if(RandomRange(MIN_MEAN, MAX_MEAN) < ((double)(MIN_MEAN + MAX_MEAN) / 50.0))
			waiting_time *= 40;

		#ifdef SIMULATE
		cumulated_time += waiting_time/1000000.0;
		cumulated_leak += leak/1024.0;
		printf("%f\t%llu\n", cumulated_time, cumulated_leak);
		

		if(cumulated_leak > 8000000)
			exit(0);

		#else
		usleep(waiting_time);
		printf(KRED "LEAK" RESET ": Leaking %.02f KB and sleeping %.02f seconds\n", (double)leak/1024, waiting_time/1000000);
		void *ptr = malloc(leak);
		if(ptr == NULL)
			continue;
		memset(ptr, 'a', leak); // To avoid zero pages to eat away the leak
		#endif
	}
	
	return 0;
}
