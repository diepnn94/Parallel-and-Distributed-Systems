#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


/*Global variables */
int num_threads; 
pthread_mutex_t *mutexes;


typedef enum{
	none,   // No forks
	one,    // One fork
	two     // Both forks to consume
} utensil;

/* Representation of a philosopher */
typedef struct phil_data{
	int phil_num;
	int course;
	utensil forks; 
}phil_data;

/* ****************Change function below ***************** */
void *eat_meal(void* philosopher_info ){
	phil_data* philosopher;
	philosopher = (phil_data*) philosopher_info;
	while(philosopher-> course < 3){
		if(pthread_mutex_trylock(&mutexes[philosopher->phil_num])== 0){
			int index = philosopher->phil_num; 
			if(pthread_mutex_trylock(&mutexes[(index+1) %(num_threads-1)]) == 0){
				printf("Philosopher %d is eating %d course\n", philosopher-> phil_num, philosopher->course);
				philosopher->course++;
				sleep(1);
				pthread_mutex_unlock(&mutexes[index]);
				pthread_mutex_unlock(&mutexes[(index+1) % (num_threads-1)]);
			}
			else{
				printf("philosopher %d can only acquire 1 fork....\n", philosopher->phil_num);
				pthread_mutex_unlock(&mutexes[index]);
				sleep(random() % 2);
			}
		}
		printf("philosopher %d cannot acquire any forks....\n", philosopher-> phil_num);
		sleep(random()% 3);	
	}

}

/* 3 course meal: Each need to acquire both forks 3 times.
 *  First try for fork in front.
 * Then for the one on the right, if not fetched, put the first one back.
 * If both acquired, eat one course.
 */


/* ****************Add the support for pthreads in function below ***************** */
int main( int argc, char **argv ){
	int num_philosophers, error;

	if (argc < 2) {
          fprintf(stderr, "Format: %s <Number of philosophers>\n", argv[0]);
          return 0;
     }
    
 	num_philosophers = num_threads = atoi(argv[1]);
	pthread_t threads[num_threads];
	phil_data philosophers[num_philosophers]; //Struct for each philosopher
	mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*num_philosophers); //Each mutex element represent a fork



	/* Initialize structs */
	for( int i = 0; i < num_philosophers; i++ ){
		philosophers[i].phil_num = i;
		philosophers[i].course   = 0;
		philosophers[i].forks    = none;
	}
/* Each thread will represent a philosopher */
	
	for (int i = 0; i < num_philosophers; i++){
		pthread_mutex_init(&mutexes[i], NULL);
	}
	for(int i = 0; i < num_philosophers; i++){
		if (pthread_create(&threads[i], NULL, eat_meal, (void*)&philosophers[i])){
			fprintf(stderr, "ERROR\n");
		}
	}


	for(int i = 0; i < num_philosophers; i++){
		pthread_join(threads[i], NULL);
	}

	for (int i = 0; i < num_philosophers; i++){
		pthread_mutex_destroy(&mutexes[i]);
	}

/* Initialize Mutex, Create threads, Join threads and Destroy mutex */

	return 0;
}
