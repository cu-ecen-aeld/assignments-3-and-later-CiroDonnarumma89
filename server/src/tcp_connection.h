#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

/***
 * TCP connection handle
 */
typedef struct
{
    int socket;                 /*!< socket file descriptor */
    const char* client_address; /*!< client address */
    bool is_open;               /*!< connection status */
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

/***
 * Receive a message from the client
 * @param connection: connection handle
 * @param message: received message
 * @param delimiter: delimiter to stop reading the message
 * @return: true if the message was received successfully, false otherwise
 * @error: return false in case of error
 * @note: the buffer must be freed by the caller
 */
bool tcp_connection_receive_message(tcp_connection_t* connection, char** message, char delimiter);

/***
 * Send a message to the client
 * @param connection: connection handle
 * @param message: message to send
 * @param size: message size
 * @return: true if the message was sent successfully, false otherwise
 * @error: return false in case of error
 */
bool tcp_connection_send_message(tcp_connection_t* connection, const char* message, ssize_t size);

/***
 * Send a file to the client
 * @param connection: connection handle
 * @param filename: file to send
 * @return: true if the file was sent successfully, false otherwise
 * @error: return false in case of error
 */
bool tcp_connection_send_file(tcp_connection_t* connection, FILE* fd);


/***
 * Check if the connection is open
 * @param connection: connection handle
 * @return: true if the connection is open, false otherwise
 */
bool tcp_connection_is_open(const tcp_connection_t* connection);