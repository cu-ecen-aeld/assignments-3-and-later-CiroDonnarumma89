#pragma once

#include <stdio.h>
#include <stdbool.h>
#include "tcp_connection.h"

struct tcp_server_t;

typedef struct tcp_server_t tcp_server_handle_t;



/***
 * Create a server
 * @param port: port number to bind the socket to
 * @return:     server handle
 * @error:      return NULL in case of error
 */
tcp_server_handle_t* tcp_server_create(const char* port);

/***
 * Destroy the server
 * @param server: server handle
 */
void tcp_server_destroy(tcp_server_handle_t* server);

/***
 * Listen for incoming connections
 * @param server: server handle
 * @return: true if the server is listening, false otherwise
 */
bool tcp_server_listen(tcp_server_handle_t* server);

/***
 * Accept a connection on the server
 * @param server: server handle
 * @return: connection handle
 * @error: return NULL in case of error
 * @note:  the connection handle should be freed by the user
 */
tcp_connection_t* tcp_server_accept(tcp_server_handle_t* server);


/***
 * Handle the connection
 * @param connection_sd: connection file descriptor
 * @param fd: file descriptor to write the data to
 * @error: exit the program in case of error
 */
void handle_connection(int connection_sd, FILE* fd);