/**
 * @file flagger.c
 * @brief Program entry point.  Runs the flagger ahead simulation
 *
 * Course: CSC3210
 * Section: YOUR SECTION HERE
 * Assignment: Flagger Ahead
 * Name: YOUR NAME HERE
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

//Directions
#define RIGHT_TO_LEFT 0
#define LEFT_TO_RIGHT 1

//Mutex and Conditions
pthread_mutex_t direction_flag = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t direction_changed = PTHREAD_COND_INITIALIZER;

//Semnaphore for capacity
sem_t capacity;

// global variables to make my life easier
volatile int running = 1;
volatile int done_cars = 0;
volatile int num_in_zone = 0;
volatile int current_direction = LEFT_TO_RIGHT;

// Custom structure to define the needed info for the car threads
struct Car{
	int id;
    int direction;
    int time_to_cross;
    int sleep_time;
    int crossing_count;
    sem_t capacity_sem;
};

/**
 * @brief This method is used to signal the other threads that a direction is now safe to drive in
 * @param direction The direction to deem safe for drivers
*/
void signal_safe_to_drive(int direction){
	if(direction == LEFT_TO_RIGHT) {
		printf("Flagger indicating safe to drive LEFT->RIGHT\n");
    } else {
		printf("Flagger indicating safe to drive RIGHT->LEFT\n");     
    }
    pthread_cond_broadcast(&direction_changed);
}

/**
 * @brief This method is used to signal the other threads that it is now unsafe to drive
*/
void signal_unsafe_to_drive(){
	printf("Flagger indicating unsafe to drive\n");
	pthread_cond_broadcast(&direction_changed);
}

/**
 * @brief This method is used to wait until all the other cars have left the construction zone
*/
void wait_until_construction_zone_cleared(){
	while(num_in_zone != 0){};
}

/**
 * @brief This method is used by the flagger thread to define the behavior of the flagger. See readme for more details.
 * @param args The arguments for the flagger thread. i.e the time it takes until we switch 
*/
void* flagger_thread(void* args) {

    int* flag_args = (int*)args;

	int allowed_drive_time = flag_args[0];
    int total_cars = flag_args[1];
    
    int direction = LEFT_TO_RIGHT;
    
    // While cars still need to pass
    //   - NOTE this will have to be set correctly in main
    while(1 == running) {
        
        // Signal to all waiting cars that it is safe to drive in the current direction
        signal_safe_to_drive(direction);
        
        // Allow cars to pass
        usleep(allowed_drive_time);
        
        // Signal to all cars that the direction is going to be flipped
        signal_unsafe_to_drive();
        
        // Wait for cars to leave the construction zone
        wait_until_construction_zone_cleared();
        pthread_mutex_lock(&direction_flag);
        // Flip the direction
        if(direction == LEFT_TO_RIGHT) {
            direction = RIGHT_TO_LEFT;
            current_direction = RIGHT_TO_LEFT;
        } else {
            direction = LEFT_TO_RIGHT;
            current_direction = LEFT_TO_RIGHT;
        }
        pthread_mutex_unlock(&direction_flag);

		if (done_cars == total_cars){
			running = 0;
		} 
    }
	printf("Flagger has finished\n");
	return NULL;
}

/**
 * @brief This method is used to have a thread wait until it is safe based on the direction they need to drive
 * @param direction The direction the car need to wait to be safe so they can drive
*/
void wait_until_safe_to_drive(int direction){
    while(direction != current_direction){
        pthread_cond_wait(&direction_changed, &direction_flag);
    }
}

/**
 * @brief This method is used to signal that they are in the construction zone to the semnaphore 
 * and to let the flagger know there is cars in the zone
 * @param direction The direction the car is driving
*/
void signal_enter_construction_zone(int direction){
	sem_wait(&capacity);
	num_in_zone++;
}

/**
 * @brief This method is used to signal that they are out of the construction zone to the semnaphore 
 * and to let the flagger know there one less car in the zone
 * @param direction The direction the car is driving
*/
void signal_exit_construction_zone(int direction){
	num_in_zone--;
	sem_post(&capacity);
}

/**
 * @brief This method is used by the car threads to define the behavior of the cars. See readme for more details.
 * @param args The arguments for the car thread. This is a custom car struct that contains all the information about a given car
*/
void* car_thread(void* args) {

    struct Car* my_car = (struct Car*)args;
    
    // Keep traveling until the car has finished all needed crossings
    while(my_car->crossing_count > 0) {
        
        pthread_mutex_lock(&direction_flag);
        // Wait for the flagger to indicate that it's safe to cross
        //   - The direction of traffic flow is the wait the car needs to go
        wait_until_safe_to_drive(my_car->direction);
        
        // Tell the flagger that the car is in the zone
        //   - the car must wait before entering if there is not enough room
        signal_enter_construction_zone(my_car->direction);
        if(my_car->direction == LEFT_TO_RIGHT) {
            printf("Car %d entering construction zone traveling: LEFT->RIGHT Crossings remaining: %d\n", my_car->id, my_car->crossing_count - 1);
        } else {
            printf("Car %d entering construction zone traveling: RIGHT->LEFT Crossings remaining: %d\n", my_car->id, my_car->crossing_count - 1);
        }
        // Cross the zone
        usleep(my_car->time_to_cross);
        
        // Tell the flagger that the car is out of the zone
        signal_exit_construction_zone(my_car->direction);

        pthread_mutex_unlock(&direction_flag);
        
        // Reduce the crossing count, wait, and try to travel in the other direction
        my_car->crossing_count--;
        if(my_car->crossing_count > 0) {
            usleep(my_car->sleep_time);
            if(my_car->direction == RIGHT_TO_LEFT) {
                my_car->direction = LEFT_TO_RIGHT;
            } else {
                my_car->direction = RIGHT_TO_LEFT;
            }
        }
    }
    done_cars++;
	printf("Car %d has finished all crossings\n", my_car->id);
    return NULL;
}

/**
 * @brief Program entry procedure for the flagger ahead simulation
 */
int main(int argc, char* argv[]) {

    FILE* input = fopen(argv[1], "r");

    // Read in the simulation parameters
    int left_cars;
    int right_cars;
    int crossing_time;
    int flow_time;
    int zone_capacity;
    fscanf(input, "%d %d %d %d %d", &left_cars, &right_cars, &crossing_time, &flow_time, &zone_capacity);

	// print input arguments
	printf("Construction Zone Parameters\n");
	printf("Initial Cars - Left Side:%d Right Side:%d\n", left_cars, right_cars);
	printf("Construction Zone - Cross Time:%d Car Capacity:%d\n", crossing_time, zone_capacity);
	printf("Flagger - Time before flipping direction:%d\n", flow_time);

	// initialize semnaphore
	sem_init(&capacity, 0, zone_capacity);
	//calculate total num of cars
	int total_cars = left_cars + right_cars;
	// Create array of cars
	struct Car* carArray = (struct Car*)malloc((total_cars) * sizeof(struct Car));
	//Start car IDs
	int id = 0;
    // Left car simulation parameters
    for(int i = 0; i < left_cars; i++) {
        int crossing_count;
        int wait_time;
        fscanf(input, "%d %d", &crossing_count, &wait_time);
		//struct Car newCar = {id, LEFT_TO_RIGHT, crossing_time, wait_time, crossing_count, capacity};
		//carArray[id] = &newCar;
        carArray[id].id = id;
        carArray[id].direction = LEFT_TO_RIGHT;
        carArray[id].time_to_cross = crossing_time;
        carArray[id].sleep_time = wait_time;
        carArray[id].crossing_count = crossing_count;
        carArray[id].capacity_sem = capacity;
		printf("Car %d - Crossing Count:%d Sleep Time:%d Initial Direction:LEFT->RIGHT\n", id, crossing_count, wait_time);
		++id;
    }

    // Right car simulation parameters
    for(int i = 0; i < right_cars; i++) {
        int crossing_count;
        int wait_time;
        fscanf(input, "%d %d", &crossing_count, &wait_time);
		//struct Car newCar = {id, RIGHT_TO_LEFT, crossing_time, wait_time, crossing_count, capacity};
		//carArray[id] = newCar;
        carArray[id].id = id;
        carArray[id].direction = RIGHT_TO_LEFT;
        carArray[id].time_to_cross = crossing_time;
        carArray[id].sleep_time = wait_time;
        carArray[id].crossing_count = crossing_count;
        carArray[id].capacity_sem = capacity;
		printf("Car %d - Crossing Count:%d Sleep Time:%d Initial Direction:RIGHT->LEFT\n", id, crossing_count, wait_time);
		++id;
    }
    // close input
    fclose(input);

	// greate flagger thread
	pthread_t flag_thread_id;
    int* flagArray = (int*)malloc(2 * sizeof(int));
    flagArray[0] = flow_time;
    flagArray[1] = total_cars;
	if (-1 == pthread_create(&flag_thread_id, NULL, flagger_thread, (void*)flagArray)){
		printf("Could not create thread: %s\n", strerror(errno));
		exit(-1);
	}

	//create car threads
	pthread_t car_thread_id[total_cars - 1];
	for(int i = 0; i < total_cars; ++i){
		if (-1 == pthread_create(&car_thread_id[i], NULL, car_thread, &carArray[i])){
			printf("Could not create thread: %s\n", strerror(errno));
			exit(-1);
		}
	}

	// run threads
	pthread_join(flag_thread_id, NULL);
	for(int i = 0; i < total_cars; i++) {
		pthread_join(car_thread_id[i], NULL);
	}

	// Destroy the mutex lock and condition
	pthread_mutex_destroy(&direction_flag);
	pthread_cond_destroy(&direction_changed);
    free(carArray);
    free(flagArray);
	return 0;
}