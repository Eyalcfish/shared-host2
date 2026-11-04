#include "../include/shared_host.h"
#include "../include/internal/syscalls.h"
#include "../include/internal/read_write.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <time.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

char* get_string_with_socket_suffix(const char* name, const char* suffix) {
    int name_size = strlen(name);
    int suffix_size = strlen(suffix);

    char* page_name = (char*)malloc(name_size+suffix_size+1);
    memcpy(page_name, name, name_size);
    memcpy(page_name+name_size, suffix, suffix_size);
    page_name[name_size+suffix_size] = 0; // EOF

    return page_name;
}

char* translate_key_to_hexa_string(size_t key) {
    char* hexa_string = (char*)malloc(sizeof(size_t)*2 + 1); // 2 chars for every byte, aka hex translation
    for (int i = 0 ; i < sizeof(size_t)*2 ; i++) {
        char cur = key & 0xF; // get the first 4 bits of the key

        if (cur >= 10) {
            hexa_string[i] = 'A' + cur-10;
        } else {
            hexa_string[i] = '0' + cur;
        }

        key = key >> 4; // prepare the prepared key for the next iteration
    }

    hexa_string[sizeof(size_t)*2] = 0; // EOF

    return hexa_string;
}

void set_functions(shared_connection* connection, size_t flags) {
    if (connection->job == SHARED_HOST_CLIENT) {
        connection->send = basic_send_client;
        connection->read = basic_read_client;
    } else {
        connection->send = basic_send_server;
        connection->read = basic_read_server;
    }


    connection->claim_send = basic_zc_claim_for_send;
    connection->publish = basic_zc_publish_for_send;

    connection->claim_read = basic_zc_claim_for_read;
    connection->drop = basic_zc_drop_for_send;
}

shared_socket* create_shared_socket(const char* name) {
    shared_socket* ret = (shared_socket*)malloc(sizeof(shared_socket));
    ret->name = name;
    return ret;
}

error register_shared_socket(shared_socket* socket) {
    char* page_name = get_string_with_socket_suffix(socket->name, "_shared_host_socket");
    socket->socket_table_buffer = allocate_shm_buffer(4096, page_name); // allocate the shm socket buffer
    if (socket->socket_table_buffer == 0) { //TODO: REMOVE THIS
        shm_unlink(page_name);
        socket->socket_table_buffer = allocate_shm_buffer(4096, page_name); // allocate the shm socket buffer
    }
    free(page_name);

    return 0;
}

shared_connection* accept_shared_connection(shared_socket* socket) {
    memcpy(socket->socket_table_buffer, (void*)socket, sizeof(shared_socket)); // copy all of the socket properties
    void* buffer = socket->socket_table_buffer + sizeof(shared_socket);

    size_t prepared_key = 0;

    while (prepared_key == 0) {
        for (int i = 0 ; i < 4096/sizeof(size_t) ; i++) {
            if (atomic_load(&(((_Atomic size_t*)buffer)[i])) != 0) { // mark the new connection in the socket
                prepared_key = (((_Atomic size_t*)buffer)[i]);
                (((_Atomic size_t*)buffer)[i]) = 0;
                break;
            }
        }
        sleep(1);
    }

    size_t useless = 0;

    char* connection_table_name = translate_key_to_hexa_string(prepared_key);
    buffer = connect_to_shm_buffer(&useless, connection_table_name); // allocate the state page of the connection

    shared_connection* connection = (shared_connection*)malloc(sizeof(shared_connection));

    connection->job = SHARED_HOST_SERVER;
    connection->origin_socket = socket;
    connection->shared_state_table = buffer;
    connection->shared_state_table_name = connection_table_name;

    set_functions(connection, socket->flags);

    // initalize the two buffers
    connection->opp_payload_buffer_name = get_string_with_socket_suffix(connection->shared_state_table_name, "_client");
    connection->own_payload_buffer_name = get_string_with_socket_suffix(connection->shared_state_table_name, "_server");

    connection->opp_payload_buffer = allocate_round_shm_buffer(4096*4, connection->opp_payload_buffer_name);
    connection->own_payload_buffer = allocate_round_shm_buffer(4096*4, connection->own_payload_buffer_name);

    atomic_store((_Atomic char*)connection->shared_state_table, 1); // signal to the client that you've accepted

//     shared_message* message = (shared_message*)malloc(sizeof(shared_message));
//     message->origin = connection;
//     message->buffer = (char*)malloc(3);
//     message->size = 3;
//     ((char*)message->buffer)[0] = 'E';
//     ((char*)message->buffer)[0] = 'C';
//     ((char*)message->buffer)[0] = 0;
//
//     connection->send(message);
//     free_message(connection->read(connection));

    return connection;
}

shared_connection* connect_shared_connection(shared_socket* socket) {
    void* buffer = 0;
    char* page_name = get_string_with_socket_suffix(socket->name, "_shared_host_socket");
    for(int i = 0 ; i < 3 ; i ++) { // retry connecting 3 times
        buffer = connect_to_shm_buffer(&socket->size, page_name);
        if (buffer != 0) {
            break;
        }
        sleep(1);
    }

    size_t socket_table_size = socket->size;

    memcpy((void*)socket, buffer, sizeof(shared_socket)); // copy all of the socket properties

    buffer += sizeof(shared_socket);

    srand(time(NULL));
    size_t prepared_key = 0;

    char value_found = 1;
    while(value_found) { // search to see if the randomly generated key exists already
        prepared_key = rand();
        prepared_key += prepared_key == 0; // prepared key cannot be equal to 0
        value_found = 0;
        for (int i = 0 ; i < socket_table_size/sizeof(size_t) ; i++) {
            if (((size_t*)buffer)[i] == prepared_key) {
                value_found = 1;
            }
        }
    }

    size_t expected = 0;

    for (int i = 0 ; i < socket_table_size/sizeof(size_t) ; i++) {
        if (atomic_compare_exchange_strong(&(((_Atomic size_t*)buffer)[i]), &expected, prepared_key)) { // mark the new connection in the socket
            break;
        }
    }

    buffer -= sizeof(shared_socket); // close the mapping of the socket
    free_shm_buffer(buffer, page_name);
    free(page_name);

    char* connection_table_name = translate_key_to_hexa_string(prepared_key);
    buffer = allocate_shm_buffer(4096, connection_table_name); // allocate the state page of the connection

    // TODO: put public keys inside the state page

    shared_connection* connection = (shared_connection*)malloc(sizeof(shared_connection));

    connection->job = SHARED_HOST_CLIENT;
    connection->origin_socket = socket;
    connection->shared_state_table = buffer;
    connection->shared_state_table_name = connection_table_name;

    set_functions(connection, socket->flags);

    while(atomic_load((_Atomic char*)(connection->shared_state_table)) == 0) {
        sleep(1); // wait for the accept action
    }

    connection->opp_payload_buffer_name = get_string_with_socket_suffix(connection->shared_state_table_name, "_server");
    connection->own_payload_buffer_name = get_string_with_socket_suffix(connection->shared_state_table_name, "_client");

    connection->opp_payload_buffer = connect_to_round_shm_buffer(&connection->origin_socket->size, connection->opp_payload_buffer_name);
    connection->own_payload_buffer = connect_to_round_shm_buffer(&connection->origin_socket->size, connection->own_payload_buffer_name);

//     shared_message* message = (shared_message*)malloc(sizeof(shared_message));
//     message->origin = connection;
//     message->buffer = (char*)malloc(3);
//     message->size = 3;
//     ((char*)message->buffer)[0] = 'E';
//     ((char*)message->buffer)[0] = 'C';
//     ((char*)message->buffer)[0] = 0;
//
//     free_message(connection->read(connection));
//     connection->send(message);

    return connection;
}

error close_connection(shared_connection* connection) {
    if (atomic_load((_Atomic char*)connection->shared_state_table) == 2) {
        free_shm_buffer_and_close_file(connection->shared_state_table,connection->shared_state_table_name);
        free_round_shm_buffer_and_close_file(connection->own_payload_buffer, connection->own_payload_buffer_name);
        free_round_shm_buffer_and_close_file(connection->opp_payload_buffer, connection->opp_payload_buffer_name);
        free(connection->shared_state_table_name);
        free(connection->opp_payload_buffer_name);
        free(connection->own_payload_buffer_name);
        if (connection->job == SHARED_HOST_CLIENT) {
            free(connection->origin_socket);
        }
        free(connection);
    } else {
        atomic_store((_Atomic char*)connection->shared_state_table, 2);
        free_shm_buffer(connection->shared_state_table,connection->shared_state_table_name);
        free_round_shm_buffer(connection->own_payload_buffer, connection->own_payload_buffer_name);
        free_round_shm_buffer(connection->opp_payload_buffer, connection->opp_payload_buffer_name);
        free(connection->shared_state_table_name);
        free(connection->opp_payload_buffer_name);
        free(connection->own_payload_buffer_name);
        if (connection->job == SHARED_HOST_CLIENT) {
            free(connection->origin_socket);
        }
        free(connection);
    }
    return 0;
}

error unregister_socket(shared_socket *socket) {
    char* page_name = get_string_with_socket_suffix(socket->name, "_shared_host_socket");
    free_shm_buffer_and_close_file(socket->socket_table_buffer, page_name); // allocate the shm socket buffer
    free(page_name);
    return 0;
}

error free_message(shared_message* message) {
    free(message->buffer);
    free(message);
    return 0;
}
