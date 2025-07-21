#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
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
        connection = NULL;
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
        connection->client_address = NULL;
        free(connection);
    }
}

bool tcp_connection_is_open(const tcp_connection_t* connection)
{
    return connection->is_open;
}


bool tcp_connection_receive_message(tcp_connection_t* connection, char** message, char delimiter)
{
    assert(connection);
    assert(message);

    int   buffer_size = 1024;
    char* buffer = (char*)malloc(buffer_size);
    assert(buffer);

    int   current_size = 0;
    char* new_line_ptr = NULL;

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
                    assert(buffer);
                }
            }
        }
        else if (recv_size == 0)
        {
            *message = NULL;
            connection->is_open = false;
            free(buffer);
            buffer = NULL;
            return false;
        }
        else
        {
            *message = NULL;
            free(buffer);
            buffer = NULL;
            perror("recv");
            return false;
        }
    } while(recv_size > 0);

}


bool tcp_connection_send_message(tcp_connection_t* connection, const char* message, ssize_t size)
{
    ssize_t ret = send(connection->socket, message, size, 0);
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

bool tcp_connection_send_file(tcp_connection_t* connection, const char* filename)
{
    const unsigned int buffer_size = 4096;
    char  buffer[buffer_size];
    ssize_t  bytes_read;
    FILE*   fd = fopen(filename, "r");

    if (fd == NULL)
    {
        perror(filename);
        return false;
    }

    while ((bytes_read = read(fd->_fileno, buffer, buffer_size)) > 0) {
        if (false == tcp_connection_send_message(connection, buffer, bytes_read))
        {
            return false;
        }
    }

    fclose(fd);
    return true;
}
