#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <unistd.h>
#include "server_mng.h"
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
    
    


    int socketfd = create_and_bind_socket("9000");

    if (socketfd == -1)
    {
        perror("server: failed to create and bind socket");
        syslog(LOG_ERR, "Failed to create and bind socket");
        fclose(fd);
        closelog();
        exit(-1);
    }

    if (listen(socketfd, 30) == -1)
    {
        perror("server: failed to listen");
        syslog(LOG_ERR, "Failed to listen");
        close(socketfd);
        fclose(fd);
        closelog();
        exit(-1);
    }

    while(!exit_program)
    {
        int connection_sd = wait_for_connection(socketfd);    
        handle_connection(connection_sd, fd);
        close(connection_sd);
    }

    close(socketfd);
    fclose(fd);
    closelog();

    return 0;
}
