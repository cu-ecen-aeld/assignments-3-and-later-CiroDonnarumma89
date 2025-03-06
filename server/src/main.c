#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include "tcp_server.h"
// Global variables
bool exit_program = false; // flag to exit the program

/***
 * Main function
 * @param argc: number of arguments
 * @param argv: arguments
 * @error: exit the program in case of error
 */
int main(int argc, char const *argv[])
{
    FILE* fd = NULL;
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

    fd = fopen("/var/tmp/aesdsocketdata", "a+");


    while(1)
    {
        printf("Server: waiting for connections...\n");
        syslog(LOG_DEBUG, "Server: waiting for connections...");
        tcp_connection_t* connection = tcp_server_accept_connection(server);
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

    fclose(fd);
    tcp_server_destroy(server);
    closelog();

    return 0;
}