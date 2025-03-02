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

    if (!tcp_server_listen(server))
    {
        syslog(LOG_ERR, "Failed to listen");
        tcp_server_destroy(server);
        exit(-1);
    }

    printf("Server: waiting for connections...\n");
    syslog(LOG_DEBUG, "Server: waiting for connections...");
    tcp_connection_t* connection = tcp_server_accept(server);
    printf("Accepted connection from from %s\n", connection->client_address);
    syslog(LOG_DEBUG, "Accepted connection from from %s\n", connection->client_address);


    tcp_connection_destroy(connection);  
    tcp_server_destroy(server);


    

    // if (socketfd == -1)
    // {
    //     perror("server: failed to create and bind socket");
    //     syslog(LOG_ERR, "Failed to create and bind socket");
    //     fclose(fd);
    //     closelog();
    //     exit(-1);
    // }

    // if (listen(socketfd, 30) == -1)
    // {
    //     perror("server: failed to listen");
    //     syslog(LOG_ERR, "Failed to listen");
    //     close(socketfd);
    //     fclose(fd);
    //     closelog();
    //     exit(-1);
    // }

    // while(!exit_program)
    // {
    //     int connection_sd = wait_for_connection(socketfd);    
    //     handle_connection(connection_sd, fd);
    //     close(connection_sd);
    // }

    //close(socketfd);
    //fclose(fd);
    closelog();

    return 0;
}
