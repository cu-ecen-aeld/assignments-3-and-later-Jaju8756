#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    
	// 1. casting void* to *thread_data to let the compiler know the fields in the struct thread_data 
    struct thread_data* thread_func_args = (struct thread_data *) thread_param; // thread_func_args and thread_param points to the same memory location
	
	// 2. Assume False until success
	thread_func_args->thread_complete_success = false;

 	// 3. wait, obtain mutex, wait, release mutex
	usleep(thread_func_args->wait_to_obtain_ms * 1000);	// milliseconds delay

	if(pthread_mutex_lock(thread_func_args->mutex) != 0){
		ERROR_LOG("Mutex lock failure");
		return thread_func_args;
	}
	
	usleep(thread_func_args->wait_to_release_ms * 1000);	// milliseconds delay
	
	if(pthread_mutex_unlock(thread_func_args->mutex) != 0){
		ERROR_LOG("Mutex lock failure");
		return thread_func_args;
	}

    // 4. Success
	thread_func_args->thread_complete_success = true;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
   	
   	
	// 1. Allocate memory for thread_data
    struct thread_data *data_struct = malloc(sizeof(struct thread_data));
    if (data_struct == NULL) {
        ERROR_LOG("malloc failed");
        return false;
    }

	// 2. setup mutex and wait arguments
	data_struct->mutex = mutex;
	data_struct->wait_to_obtain_ms = wait_to_obtain_ms;
	data_struct->wait_to_release_ms = wait_to_release_ms;
	data_struct->thread_complete_success = false;

	// 3. pass thread_data to created thread
	if(pthread_create(thread, NULL, threadfunc, data_struct)){
		ERROR_LOG("Thread creation failed");
		free(data_struct);			// on successful creation, the thread owns the right to use data_struct. This function must NOT free it
		return false;
	}
    
    return true;
}

