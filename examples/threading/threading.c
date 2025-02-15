#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <sys/time.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

bool wait_ms(unsigned int ms)
{
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = ms * 1000000
    };

    int ret = 0;

    DEBUG_LOG("Waiting for %d ms", ms);

    ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

    return (ret == 0);
}

void* threadfunc(void* thread_param)
{
    struct timeval t1, t2;

    struct thread_data* my_args = (struct thread_data*) thread_param;
    my_args->thread_complete_success = false;

    gettimeofday(&t1, NULL);
    if (!wait_ms(my_args->ms_to_wait_before_lock))
    {
        ERROR_LOG("Unable to wait before locking");
        return my_args;
    }
    gettimeofday(&t2, NULL);
    DEBUG_LOG("Time elapsed waiting before locking: %ld\n", (t2.tv_usec - t1.tv_usec) / 1000);


    if (0 != pthread_mutex_lock(my_args->p_mutex))
    {
        ERROR_LOG("Unable to lock the mutex");
        return my_args;
    }

    DEBUG_LOG("I am in the critical section");

    gettimeofday(&t1, NULL);
    if (!wait_ms(my_args->ms_to_wait_before_unlock))
    {
        ERROR_LOG("Unable to wait before unlocking");
        return my_args;
    }
    gettimeofday(&t2, NULL);
    DEBUG_LOG("Time elapsed waiting before unlocking: %ld\n", (t2.tv_usec - t1.tv_usec) / 1000);


    if (0 != pthread_mutex_unlock(my_args->p_mutex))
    {
        ERROR_LOG("Unable to unlock the mutex");
        return my_args;
    }

    DEBUG_LOG("I am outside the  critical section");

    my_args->thread_complete_success = true;
    return my_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* thread_data = (struct thread_data*) malloc(sizeof(struct thread_data));

    if (NULL == thread_data)
    {
        ERROR_LOG("Unable to allocate memory for thread_data");
        return false; 
    }

    thread_data->ms_to_wait_before_lock = wait_to_obtain_ms;
    thread_data->ms_to_wait_before_unlock = wait_to_release_ms;
    thread_data->p_mutex = mutex;

    DEBUG_LOG("thread_data has been initialized");



    if (0 != pthread_create(thread, NULL, threadfunc, thread_data))
    {
        ERROR_LOG("Unable to create the thread");
        return false;
    }
    else
    {
        DEBUG_LOG("Thread has been created");
        return true;
    }
}

