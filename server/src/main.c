#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <linux/fs.h>
#include <linux/limits.h>

#include "connection_manager.h"
#include "tcp_server.h"

#ifdef USE_AESD_CHAR_DEVICE
#define TARGET_FILENAME "/dev/aesdchar"
#else
#define TARGET_FILENAME "/var/tmp/aesdsocketdata"
#endif

// Global variables

const char*             filename            = TARGET_FILENAME;              /*!< file to save the messages */
bool                    exit_program        = false;                        /*!< flag to exit the program */
connection_manager_t*   connection_manager  = NULL;                         /*!< connection manager */
// Data structures


// Function prototypes

static bool daemonize(void);
static void setup(void);
static void teardown(void);
static void signal_handler(int signal_number);
static void save_message(char* message, const char* filename);

/***
 * Main function
 * @param argc: number of arguments
 * @param argv: arguments
 * @error: exit the program in case of error
 */
int main(int argc, char const *argv[])
{
    setup();

    if ((argc == 2) && (0 == strcmp(argv[1], "-d")))
    {
        printf("Trying to create a daemon\n");
        if (true == daemonize())
        {
            printf("I am a daemon\n");
        }
        else
        {
            printf("Failed to create a daemon\n");
        }
    }

    connection_manager = connection_manager_create(filename);
    assert(connection_manager);

    tcp_server_handle_t* server = tcp_server_create("9000");

    if (server == NULL)
    {
        syslog(LOG_ERR, "Failed to create server");
        exit(-1);
    }

    if (!tcp_server_listen_for_connections(server))
    {
        syslog(LOG_ERR, "Failed to listen");
        tcp_server_destroy(server);
        server = NULL;
        exit(-1);
    }

    while(!exit_program)
    {
        printf("Server: waiting for connections...\n");
        syslog(LOG_DEBUG, "Server: waiting for connections...");
        tcp_connection_t* connection = tcp_server_accept_connection(server);
        if (connection != NULL)
        {
            printf("Accepted connection from %s\n", connection->client_address);
            syslog(LOG_DEBUG, "Accepted connection from from %s\n", connection->client_address);
            connection_manager_register_connection(connection_manager, connection);
        }
    }

    printf("Caught signal, exiting\n");
    tcp_server_destroy(server);
    connection_manager_destroy(connection_manager);
    connection_manager = NULL;
    teardown();


    return 0;
}

static bool daemonize(void)
{
    pid_t pid;
    int i;

    /* create new process */
    pid = fork();
    if (pid == -1)
        exit(EXIT_FAILURE);
    else if (pid != 0)
        exit (EXIT_SUCCESS);

    /* create new session and process group */
    if (setsid() == -1)
        return false;

    /* set the working directory to the root directory */
    if (chdir("/") == -1)
        return false;

    // /* close all open files */
    // for (i = 0; i < 1024; i++)
    //     close(i);

    /* redirect fd's 0,1,2 to /dev/null */
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    return true;
}

static void setup(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler=signal_handler;
    if (sigaction(SIGTERM, &action, NULL) != 0)
    {
        printf("Error %d (%s) registering for SIGTERM", errno, strerror(errno));
        exit(-1);
    }
    if (sigaction(SIGINT, &action, NULL) != 0)
    {
        printf("Error %d (%s) registering for SIGINT", errno, strerror(errno));
        exit(-1);
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
}

static void teardown(void)
{
    #ifndef USE_AESD_CHAR_DEVICE
    if (remove(filename) == 0) {
        printf("%s deleted successfully.\n", filename);
    } else {
        printf("Error: Unable to delete %s.\n", filename);
    }
    #endif
    closelog();
}


static void signal_handler(int signal_number)
{
    syslog(LOG_DEBUG, "Caught signal, exiting");
    exit_program = true;
}

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
