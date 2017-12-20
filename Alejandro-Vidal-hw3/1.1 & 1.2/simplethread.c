
/********************************************************
##########################################################
## COP4610 – Principles of Operating Systems – Fall 2017
## 
## Student: Alejandro Vidal – 5913959
## Project: Multithreaded Programming
##
## I Certify that this program code has been written by me
## and no part of it has been taken from any sources.
##########################################################
*********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

int SharedVariable = 0;
void *SimpleThread(void *param);

#ifdef PTHREAD_SYNC
pthread_mutex_t my_mutex;
pthread_barrier_t my_barrier;
#endif

int main(int argc, char **argv) {
	
	int value, i;	
	pthread_t *my_threads;	

	//validate if number of threads was passed as a parameter, if not exit with error
	if(argc < 2){
		printf("You must provide the number of threads you wish to create as a parameter\n");
		exit(1);
	}
	
	value = atoi(argv[1]);
	if(value == 0){ /* validate the parameter*/
		printf("You must enter an integer greater than 0\n");	
		exit(2);
	}
	
	//allocate memory for the array of threads
	my_threads = malloc(sizeof(pthread_t) * value);
	
	//initialize the mutex and the pthread barrier
	#ifdef PTHREAD_SYNC
	pthread_mutex_init(&my_mutex, NULL);
    	pthread_barrier_init(&my_barrier, NULL, value);
	#endif
	
	//create the threads
	for(i = 0; i < value; i++){
		pthread_create(&my_threads[i], NULL, SimpleThread, (void*) (intptr_t) i);
	}

	
	//join the threads
	for(i = 0; i < value; i++){
		pthread_join(my_threads[i],NULL);
	}

	#ifdef PTHREAD_SYNC
	pthread_barrier_destroy(&my_barrier);
        pthread_mutex_destroy(&my_mutex);
	#endif
	
	exit(0);
}

void *SimpleThread(void *param) { 
        
	int num, which, val = 0;
	which = (intptr_t) param;
	for(num = 0; num < 20; num++) { 
		if (random() > RAND_MAX / 2) 
			usleep(10);

		#ifdef PTHREAD_SYNC
		pthread_mutex_lock (&my_mutex);
		#endif
 
		val = SharedVariable;

		printf("*** thread %d sees value %d\n", which, val); SharedVariable = val + 1;
		
		#ifdef PTHREAD_SYNC
		pthread_mutex_unlock (&my_mutex);
		#endif
	}

	#ifdef PTHREAD_SYNC
	pthread_barrier_wait(&my_barrier);	
	#endif	

	val = SharedVariable;
	printf("Thread %d sees final value %d\n", which, val);
}