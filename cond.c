#include <pthread.h>
#include <time.h>
#include "errors.h"

typedef struct my_struct_tag {
    pthread_mutex_t mutex; // Protects access to value
    pthread_cond_t cond; // Signals change to value
    int value; // Access protected by mutex
} my_struct_t;

my_struct_t data = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0 };

int hibernation = 1; // Default to 1 second

/*
 * Thread start routine. It will set the main thread's predicate
 * and signal the conditional variable.
 */
void* wait_thread(void* arg) {
    int status;

    status = pthread_mutex_lock(&data.mutex);
    if (status != 0)
        err_abort(status, "Lock mutex");
    data.value = 1; // Set predicate
    status = pthread_cond_signal(&data.cond);
    if (status != 0)
        err_abort(status, "Signal condition");
    status = pthread_mutex_unlock(&data.mutex);
    if (status != 0)
        err_abort(status, "Unlock mutex");
    return NULL;
}

int main(int argc, char** argv) {
    int status;
    pthread_t thread_wait_id;
    struct timespec timeout;

    /*
     * If an argument is specified, interpret it as a number
     * of seconds for wait_thread to sleep before signaling the
     * condition variable. You can play with this to see the
     * condition wait below timeout or wake normally.
     */
    if (argc > 1)
        hibernation = atoi(argv[1]);
    
    /*
     * Creat wait thread.
     */
    status = pthread_create(&thread_wait_id, NULL, wait_thread, NULL);
    if (status != 0)
        err_abort(status, "Create wait thread");
    
    /*
     * Wait on the conditional variable for 2 seconds, or until
     * signaled be the wait_thread. Normally, wait_thread
     * should signal. If your raise "hibernation" above 2 
     * seconds, it will timeout.
     */
    timeout.tv_sec = time(NULL) + 2;
    timeout.tv_nsec = 0;
    status = pthread_mutex_lock(&data.mutex);
    if (status != 0)
        err_abort(status, "Lock mutex");
    
    while (data.value == 0) {
        status = pthread_cond_timedwait(&data.cond, &data.mutex, &timeout);
        if (status == ETIMEDOUT) {
            printf("Condition wait timed out\n");
            break;
        }
        if (status != 0)
            err_abort(status, "Wait on condition");
    }

    if (data.value != 0)
        printf("Condition was signaled\n");
    
    status = pthread_mutex_unlock(&data.mutex);
    if (status != 0)
        err_abort(status, "Unlock mutex");
    return 0;
}