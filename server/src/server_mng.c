#include "server_mng.h"
#include <syslog.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

int wait_for_connection(int socketfd)
{
    char s[INET6_ADDRSTRLEN]; 
    struct sockaddr client_addrress;
    socklen_t client_addrress_len = sizeof(client_addrress);
    
    printf("server: waiting for connections...\n");
    int connection_sd = accept(socketfd, &client_addrress, &client_addrress_len);
    if (connection_sd == -1)
    {
        perror("server: failed to accept");
        syslog(LOG_ERR, "Failed to accept");
    }
    printf("accepted\n\r");
    inet_ntop(client_addrress.sa_family, get_in_addr((struct sockaddr *)&client_addrress), s, sizeof(s));
    printf("Accepted connection from from %s\n", s);
    syslog(LOG_DEBUG, "Accepted connection from from %s\n", s);    
    
    return connection_sd;
}



int create_and_bind_socket(const char* port)
{
    int ret;
    int socketfd = -1;
    struct addrinfo hints, *res = NULL;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family     = AF_UNSPEC;   // IPv4 or IPv6
    hints.ai_socktype   = SOCK_STREAM; // TCP
    hints.ai_flags      = AI_PASSIVE;  // Use my IP
    
    ret = getaddrinfo(NULL, port, &hints, &res);
    
    if (0 == ret)
    {    
        // getaddrinfo succeeded
        bool succeeded = false;
        
        // loop through all the results and bind to the first we can
        for (struct addrinfo *it = res; (it != NULL) && (!succeeded); it = it->ai_next)
        {
            socketfd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            
            int yes = 1;
            if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("setsockopt");
                syslog(LOG_ERR, "Failed to set socket options");
                close(socketfd);
                continue;
            }

            if (socketfd == -1)
            {
                perror("server: socket");
                syslog(LOG_ERR, "Failed to create socket");
                continue;
            }
                
            if (bind(socketfd, it->ai_addr, it->ai_addrlen) == -1)
            {
                close(socketfd);
                perror("server: bind");
                syslog(LOG_ERR, "Failed to bind");
                continue;
            }
        
            // if we got here, we have a valid socket and it is bound    
            succeeded = true;
        }
    
        // no longer need the linked list of addrinfo
        freeaddrinfo(res);
        
        if (!succeeded)
        {
            socketfd = -1;
            fprintf(stderr, "server: failed to create and bind\n");
            syslog(LOG_ERR, "Failed to create and bind");
            return -1;
        }
    }
    else
    {
        // getaddrinfo failed
        fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(ret));
        syslog(LOG_ERR, "Failed to get address info: %s", gai_strerror(ret));
        return -1;
    }
    
    return socketfd;
}

void save_packet(char *packet)
{
    FILE* fd = fopen("/var/tmp/aesdsocketdata", "a");
    if (fd == NULL)
    {
        perror("/var/tmp/aesdsocketdata");
        syslog(LOG_ERR, "Unable to create or open file: %s", "/var/tmp/aesdsocketdata");
        exit(-1);
    }

    fprintf(fd, "%s\n", packet);
    fclose(fd);
}

void send_all_to_client(int connection_sd)
{
    FILE* fd = fopen("/var/tmp/aesdsocketdata", "r");
    if (fd == NULL)
    {
        perror("/var/tmp/aesdsocketdata");
        syslog(LOG_ERR, "Unable to open file: %s", "/var/tmp/aesdsocketdata");
        exit(-1);
    } 

    //send all file line by line
    char line[256];
    while (fgets(line, sizeof(line), fd))
    {
        send(connection_sd, line, strlen(line), 0);
    }
    fclose(fd);
}

void handle_connection(int connection_sd, FILE* fd)
{
    int   buffer_size = 1024;
    char* buffer = (char*)malloc(buffer_size);

    int   current_size = 0;
    char* new_line_ptr;

    int   recv_size;
    do {
        recv_size = recv(connection_sd, buffer + current_size, 1, 0);
        if (recv_size > 0)
        {
            if (buffer[current_size] == '\n')
            {
                buffer[current_size] = '\0';
                printf("received: %s\n", buffer);
                syslog(LOG_DEBUG, "Received: %s", buffer);
                save_packet(buffer);
                send_all_to_client(connection_sd);
                current_size = 0;
            }
            else
            {
                current_size++;
                if (current_size == buffer_size)
                {
                    buffer_size *= 2;
                    buffer = (char*)realloc(buffer, buffer_size);
                    if (buffer == NULL)
                    {
                        perror("realloc");
                        syslog(LOG_ERR, "Failed to reallocate buffer");
                        exit(-1);
                    }
                }
            }
        }
        else if (recv_size == 0)
        {
            printf("Connection closed\n");
            syslog(LOG_DEBUG, "Connection closed");
        }
        else
        {
            perror("recv");
            syslog(LOG_ERR, "Failed to receive data");
        }
    } while(recv_size > 0);
}


/***
 * Get sockaddr, IPv4 or IPv6
 * @param sa: sockaddr structure
 * @return: pointer to the address
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    else
    {
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }

}

