#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
