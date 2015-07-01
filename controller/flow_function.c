#include "flow_function.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_vector(float *v, int size) {
	int x = 0;
	for(x = 0 ; x < size ; x++) {
		printf("%f\t", v[x]);
	}
	printf("\n");
}

void print_matrix(float *M, int size) {
	int x = 0;
	int y = 0;

	for(x = 0 ; x < size ; x++) {
		printf(" (");
		for(y = 0 ; y < size ; y++){
			printf("%f\t", M[x*size+y]);
		}
		printf(")\n");
	}
}

void calculate_flow_matrix(float *M, float *f, float *p, int size) {
	int i=0, j=0, current=0;
	float residual, sum;
	float * f_residual = (float*) malloc(sizeof(float)*size);
	memcpy(f_residual,f,sizeof(float)*size);
	memset(M,0,sizeof(float)*size*size);
	for (i=0; i<size; i++) {
		if (f[i] <= p[i]) {
			M[i*size+i]=f[i];
			f_residual[i]=0;
		} else {
			M[i*size+i]=p[i];
			f_residual[i] = f[i] - p[i];
		}
	}
	

	for (i=0; i<size; i++) {
		current =0;
		while (f_residual[i]>0) {
			if (i!=current) {
				//sum column [index]
				sum=0;
				for (j=0; j<size; j++) {
					sum+=M[j*size+current];
				}
				residual=p[current]-sum;
				if (residual>f_residual[i]) {
					M[i*size+current]+=f_residual[i];
					f_residual[i]=0;
				} else if (residual>0) {
					M[i*size+current]+=residual;
					f_residual[i] -= residual;
				}
			}
			current++;
		}
	}
	for(i=0; i<size; i++)
		if(f[i] != 0){	
			for(j=0; j<size; j++){
				M[i*size+j] /= f[i];
			}
		}
}
