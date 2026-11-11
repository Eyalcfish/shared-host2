#ifndef READ_WRITE_H
#define READ_WRITE_H

#include "../shared_host.h"

typedef struct StateTable {
    char flag; // 0: connected but not accepted, 1: connected and talking, 2: closed connection
    size_t client_write_offset; // client producer server consumer
    size_t server_read_offset;

    char padding[64];

    size_t server_write_offset; // server producer client consumer
    size_t client_read_offset;
} StateTable;

error basic_send_client(shared_message* message); // normal send client
error basic_send_server(shared_message* message); // normal send server

shared_message* basic_read_client(shared_connection* connection); // normal read client
shared_message* basic_read_server(shared_connection* connection); // normal read server

shared_message* basic_zc_claim_for_send(shared_connection* connection, size_t size); // zero copy send claim
error basic_zc_publish_for_send(shared_message* message); // zero copy send publish

shared_message* basic_zc_claim_for_read(shared_connection* connection); // zero copy read claim
error basic_zc_drop_for_send(shared_message* message); // zero copy read free

#endif
