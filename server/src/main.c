#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "tcp_server.h"
// Global variables
const char* filename = "/var/tmp/aesdsocketdata";
bool exit_program = false; // flag to exit the program



static void signal_handler(int signal_number)
{
    syslog(LOG_DEBUG, "Caught signal, exiting");
    exit_program = true;
}

/***
 * Main function
 * @param argc: number of arguments
 * @param argv: arguments
 * @error: exit the program in case of error
 */
int main(int argc, char const *argv[])
{
    FILE* fd = NULL;
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
        exit(-1);
    }

    fd = fopen(filename, "a+");


    while(!exit_program)
    {
        printf("Server: waiting for connections...\n");
        syslog(LOG_DEBUG, "Server: waiting for connections...");
        tcp_connection_t* connection = tcp_server_accept_connection(server);
        if (connection == NULL)
        {
            continue;
        }
        printf("Accepted connection from %s\n", connection->client_address);
        syslog(LOG_DEBUG, "Accepted connection from from %s\n", connection->client_address);
        char* message = NULL;

        while(tcp_connection_is_open(connection))
        {
            if (tcp_connection_receive_message(connection, &message, '\n'))
            {
                fprintf(fd, "%s\n", message);
                free(message);
                tcp_connection_send_file(connection, fd);
            }
        }


        printf("Closed connection from %s\n", connection->client_address);
        syslog(LOG_DEBUG, "Closed connection from %s\n", connection->client_address);
        tcp_connection_destroy(connection);
    }

    printf("Caught signal, exiting\n");
    fclose(fd);
    tcp_server_destroy(server);

    if (remove(filename) == 0) {
        printf("%s deleted successfully.\n", filename);
    } else {
        printf("Error: Unable to delete %s.\n", filename);
    }
    closelog();

    return 0;
}