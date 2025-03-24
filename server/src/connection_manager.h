#pragma once
#include "tcp_connection.h"

struct connection_manager_t;
typedef struct connection_manager_t connection_manager_t;

connection_manager_t* connection_manager_create(const char* filename);
bool                  connection_manager_register_connection(connection_manager_t* manager, tcp_connection_t* connection);

void                  connection_manager_destroy(connection_manager_t* manager);
bool                  connection_manager_register_connection(connection_manager_t* manager, tcp_connection_t* connection);
