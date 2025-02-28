#pragma once

#include <sys/socket.h>
#include <stdio.h>

/***
 * Create a socket and bind it to the port
 * @param port: port number to bind the socket to
 * @return: socket file descriptor
 * @error: return -1 in case of error
 */
int create_and_bind_socket(const char* port);

/***
 * Get sockaddr, IPv4 or IPv6
 * @param sa: sockaddr structure
 * @return: pointer to the address
 */
void *get_in_addr(struct sockaddr *sa);

/***
 * Wait for a connection on the socket
 * @param socketfd: socket file descriptor
 * @return: connection file descriptor
 * @error: exit the program in case of error
 */
int wait_for_connection(int socketfd);

/***
 * Handle the connection
 * @param connection_sd: connection file descriptor
 * @param fd: file descriptor to write the data to
 * @error: exit the program in case of error
 */
void handle_connection(int connection_sd, FILE* fd);