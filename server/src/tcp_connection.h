#pragma once

typedef struct 
{
    int socket;
    const char* client_address;
} tcp_connection_t; 


/***
 * Create a connection
 * @param socket: socket file descriptor
 * @param client_address: client address
 * @return: connection handle
 * @error: return NULL in case of error
 */
tcp_connection_t* tcp_connection_create(int socket, const char* client_address);

/***
 * Destroy the connection
 * @param connection: connection handle
 */
void tcp_connection_destroy(tcp_connection_t* connection);
