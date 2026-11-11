#include "../../include/internal/read_write.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

error basic_send_server(shared_message* message) {
    shared_connection* connection = message->origin;
    StateTable* settings = (StateTable*)(connection->shared_state_table);

    while ((settings->server_write_offset < settings->client_read_offset && settings->server_write_offset+sizeof(void*)+message->size >= settings->client_read_offset)) {
        // loop waiting for enough storage for a new message
    }

    *(size_t*)(connection->opp_payload_buffer + settings->server_write_offset) = message->size;
    settings->server_write_offset += sizeof(size_t);
    memcpy(connection->opp_payload_buffer + settings->server_write_offset, message->buffer, message->size);
    settings->server_write_offset += message->size;

    if ((settings->server_write_offset & (1 << 14)) == 1 << 14) {
        settings->server_write_offset = 0;
    }

    return 0;
} // normal send

error basic_send_client(shared_message* message) {
    shared_connection* connection = message->origin;
    StateTable* settings = (StateTable*)(connection->shared_state_table);

    while ((settings->client_write_offset < settings->server_write_offset && settings->client_write_offset+sizeof(void*)+message->size >= settings->client_read_offset)) {
        // loop waiting for enough storage for a new message
    }

    *(size_t*)(connection->opp_payload_buffer + settings->client_write_offset) = message->size;
    settings->client_write_offset += sizeof(size_t);
    memcpy(connection->opp_payload_buffer + settings->client_write_offset, message->buffer, message->size);
    settings->client_write_offset += message->size;

    if (settings->client_write_offset >= connection->origin_socket->size) {
        settings->client_write_offset = 0;
    }

    return 0;
} // normal send

shared_message* basic_read_client(shared_connection* connection) {
    StateTable* settings = (StateTable*)(connection->shared_state_table);

    while (settings->client_read_offset == settings->server_write_offset) {
        // wait for the server to finish writing a message, or just write a message
    }



    shared_message* message = (shared_message*)malloc(sizeof(shared_message));

    message->origin = connection;

    message->size = *(size_t*)(connection->own_payload_buffer+settings->client_read_offset);
    connection->own_payload_buffer += sizeof(size_t);

    message->buffer = malloc(message->size);
    memcpy(message->buffer, (connection->own_payload_buffer+settings->client_read_offset), message->size);

    settings->client_read_offset += message->size;

    return message;
} // normal read client

shared_message* basic_read_server(shared_connection* connection) {
    StateTable* settings = (StateTable*)(connection->shared_state_table);

    while (settings->server_read_offset == settings->client_write_offset) {
        // wait for the server to finish writing a message, or just write a message
    }

    shared_message* message = (shared_message*)malloc(sizeof(shared_message));

    message->origin = connection;

    message->size = *(size_t*)(connection->own_payload_buffer+settings->server_read_offset);
    connection->own_payload_buffer += sizeof(size_t);


    message->buffer = malloc(message->size);
    memcpy(message->buffer, (connection->own_payload_buffer+settings->server_read_offset), message->size);

    settings->server_read_offset += message->size;

    return message;
} // normal read server

shared_message* basic_zc_claim_for_send(shared_connection* connection, size_t size) {
    return 0;
} // zero copy send claim

error basic_zc_publish_for_send(shared_message* message) {
    return 0;
} // zero copy send publish

shared_message* basic_zc_claim_for_read(shared_connection* connection) {
    return 0;
} // zero copy read claim

error basic_zc_drop_for_send(shared_message* message) {
    return 0;
} // zero copy read free
