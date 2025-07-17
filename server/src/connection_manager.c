#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "queue.h" // Queue taken from FreeBSD10

#include "connection_manager.h"

/***
 * Element of the connection manager list
 */
typedef struct connection_ctx_t
{
    connection_manager_t*           manager;        /*!< Connection manager */
    tcp_connection_t*               connection;     /*!< Connection */
    pthread_t                       thread;         /*!< Thread */
    SLIST_ENTRY(connection_ctx_t)   nodes;          /*!< List */
} connection_ctx_t;

/***
 * Connection manager
 */
typedef struct connection_manager_t {
    char*                                                    filename;
    pthread_mutex_t                                          file_mutex;
    #ifndef USE_AESD_CHAR_DEVICE
    timer_t                                                  timerid;
    #endif
    bool                                                     close_connections;
    SLIST_HEAD(connection_ctx_list_head_t, connection_ctx_t) connection_ctx_list;
} connection_manager_t;


static void*    thread_routine(void * connection_ctx);
static void     save_message(char* message, const char* filename);

#ifndef USE_AESD_CHAR_DEVICE
static void     thread_timer(union sigval sigval);
#endif


connection_manager_t* connection_manager_create(const char* filename)
{
    connection_manager_t* manager = (connection_manager_t*)malloc(sizeof(connection_manager_t));
    manager->filename = (char*)malloc(strlen(filename) + 1);
    strcpy(manager->filename, filename);
    pthread_mutex_init(&manager->file_mutex, NULL);
    manager->close_connections = false;
    SLIST_INIT(&manager->connection_ctx_list);

    #ifndef USE_AESD_CHAR_DEVICE
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = thread_timer;
    sev.sigev_value.sival_ptr = manager;

    timer_t timerid;
    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1)
    {
        perror("timer_create");
        syslog(LOG_ERR, "Unable to create timer");
        exit(-1);
    }
    manager->timerid = timerid;

    struct itimerspec its;
    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 10;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
    #endif


    return manager;
}

void connection_manager_destroy(connection_manager_t* manager)
{
    struct connection_ctx_t* connection_ctx = NULL;

    if (manager != NULL)
    {
        manager->close_connections = true;
        while(!SLIST_EMPTY(&manager->connection_ctx_list))
        {
            connection_ctx = SLIST_FIRST(&manager->connection_ctx_list);
            pthread_join(connection_ctx->thread, NULL);
            SLIST_REMOVE_HEAD(&manager->connection_ctx_list, nodes);
            free(connection_ctx);
            connection_ctx = NULL;
        }
        pthread_mutex_destroy(&manager->file_mutex);
        #ifndef USE_AESD_CHAR_DEVICE
        timer_delete(manager->timerid);
        #endif
        free(manager->filename);
        free(manager);
    }
}

bool connection_manager_register_connection(connection_manager_t* manager, tcp_connection_t* connection)
{
    connection_ctx_t* connection_ctx = (connection_ctx_t*)malloc(sizeof(connection_ctx_t));
    connection_ctx->connection = connection;
    connection_ctx->manager = manager;
    pthread_t                           tid;

    if (0 == pthread_create(&tid, NULL, thread_routine, (void*)connection_ctx))
    {
        connection_ctx->thread = tid;
        SLIST_INSERT_HEAD(&manager->connection_ctx_list, connection_ctx, nodes);
        return true;
    }
    else
    {
        return false;
    }
}

static void* thread_routine(void * connection_ctx)
{
    char* message = NULL;
    tcp_connection_t* connection = ((connection_ctx_t*)connection_ctx)->connection;

    while(tcp_connection_is_open(connection) && !((connection_ctx_t*)connection_ctx)->manager->close_connections)
    {
        if (tcp_connection_receive_message(connection, &message, '\n'))
        {
            pthread_mutex_lock(&((connection_ctx_t*)connection_ctx)->manager->file_mutex);
            save_message(message, ((connection_ctx_t*)connection_ctx)->manager->filename);
            tcp_connection_send_file(connection, ((connection_ctx_t*)connection_ctx)->manager->filename);
            pthread_mutex_unlock(&((connection_ctx_t*)connection_ctx)->manager->file_mutex);
            free(message);
        }
    }
    printf("Closed connection from %s\n", ((connection_ctx_t*)connection_ctx)->connection->client_address);
    syslog(LOG_DEBUG, "Closed connection from %s\n", ((connection_ctx_t*)connection_ctx)->connection->client_address);
    tcp_connection_destroy(((connection_ctx_t*)connection_ctx)->connection);
    ((connection_ctx_t*)connection_ctx)->connection = NULL;
    return 0;
}

#ifndef USE_AESD_CHAR_DEVICE
static void thread_timer(union sigval sigval)
{
    connection_manager_t* manager = (connection_manager_t*)sigval.sival_ptr;
    char outstr[200];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);

    strftime(outstr, 200, "%Y-%m-%d %H:%M:%S", tmp);
    if (0 != pthread_mutex_lock(&manager->file_mutex))
    {
        perror("pthread_mutex_lock");
        syslog(LOG_ERR, "Unable to lock file mutex");
        exit(-1);
    }
    FILE* fd = fopen(manager->filename, "a");
    if (fd == NULL)
    {
        perror(manager->filename);
        syslog(LOG_ERR, "Unable to create or open file: %s", manager->filename);
        exit(-1);
    }
    fprintf(fd, "timestamp: %s\n", outstr);
    fclose(fd);
    pthread_mutex_unlock(&manager->file_mutex);
}
#endif



static void save_message(char* message, const char* filename)
{
    FILE* fd = fopen(filename, "a");
    if (fd == NULL)
    {
        perror(filename);
        syslog(LOG_ERR, "Unable to create or open file: %s", filename);
        exit(-1);
    }

    fprintf(fd, "%s\n", message);
    fclose(fd);
}