#ifndef SHARED_HOST_H
#define SHARED_HOST_H

#include <stddef.h>

typedef int error;
typedef char SocketState;
typedef char Job; // i.e server/client

typedef struct shared_socket { // For the future i can deifne in the socket table a public key for a certain connection, and then ensure direct connection
    const char* name;
    size_t size;
    size_t flags;
    SocketState state;
} shared_socket;

typedef struct shared_connection shared_connection;

typedef struct shared_message {
    size_t size;
    void* buffer;
    shared_connection* origin; // mainly for freeing
} shared_message;

typedef struct shared_connection {
    Job job;
    shared_socket* origin_socket;
    void* shared_state_table;

    error (*send)(shared_message*); // normal send
    shared_message* (*read)(shared_connection*); // normal read

    shared_message* (*claim_send)(shared_connection*, size_t); // zero copy send claim
    error (*publish)(shared_message*); // zero copy send publish

    shared_message* (*claim_read)(shared_connection*); // zero copy read claim
    error (*drop)(shared_message*); // zero copy read free
} shared_connection;

shared_socket* create_shared_socket(const char* name); // has default size

error register_shared_socket(shared_socket* socket);

shared_connection* accept_shared_connection(shared_socket* socket);

shared_connection* connect_shared_connection(shared_socket* socket);

#define SHARED_HOST_CLIENT 0
#define SHARED_HOST_SERVER 1



#endif
