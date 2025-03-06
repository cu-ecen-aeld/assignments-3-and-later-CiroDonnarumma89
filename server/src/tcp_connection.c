#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include "tcp_connection.h"

tcp_connection_t* tcp_connection_create(int socket, const char* client_address)
{
    tcp_connection_t* connection = (tcp_connection_t*)malloc(sizeof(tcp_connection_t));
    if (connection == NULL)
    {
        return NULL;
    }

    connection->socket = socket;

    connection->client_address = strdup(client_address);
    if (connection->client_address == NULL)
    {
        free(connection);
        return NULL;
    }

    connection->is_open = true;

    return connection;
}

void tcp_connection_destroy(tcp_connection_t* connection)
{
    if (connection != NULL)
    {
        close(connection->socket);
        free((void*)connection->client_address);
        free(connection);
    }
}

bool tcp_connection_is_open(const tcp_connection_t* connection)
{
    return connection->is_open;
}


bool tcp_connection_receive_message(tcp_connection_t* connection, char** message, char delimiter)
{
    int   buffer_size = 1024;
    char* buffer = (char*)malloc(buffer_size);

    int   current_size = 0;
    char* new_line_ptr;

    int   recv_size;

    do {
        recv_size = recv(connection->socket, buffer + current_size, 1, 0);
        if (recv_size > 0)
        {
            if (buffer[current_size] == delimiter)
            {
                buffer[current_size] = '\0';
                *message = buffer;
                return true;
            }
            else
            {
                current_size++;
                if (current_size == buffer_size)
                {
                    buffer_size *= 2;
                    buffer = (char*)realloc(buffer, buffer_size);
                }
            }
        }
        else if (recv_size == 0)
        {
            *message = NULL;
            connection->is_open = false;
            free(buffer);
            return false;
        }
        else
        {
            *message = NULL;
            free(buffer);
            perror("recv");
            return false;
        }
    } while(recv_size > 0);

}


bool tcp_connection_send_message(tcp_connection_t* connection, const char* message, ssize_t size)
{
    ssize_t ret = send(connection->socket, message, strlen(message), 0);
    if (ret == -1)
    {
        perror("send");
        return false;
    }
    else
    {
        return true;
    }
}

bool tcp_connection_send_file(tcp_connection_t* connection, FILE* fd)
{
    long int pos = ftell(fd);
    rewind(fd);

    char line[256];
    while (fgets(line, sizeof(line), fd))
    {
        if (false == tcp_connection_send_message(connection, line, strlen(line)))
        {
            return false;
        }
    }

    fseek(fd, pos, SEEK_SET);
    return true;
}
