#include "../include/shared_host.h"
#include "../include/internal/syscalls.h"
#include "../include/internal/read_write.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <stdatomic.h>
#include <unistd.h>

char* get_string_with_socket_suffix(const char* name) {
    int name_size = strlen(name);
    const char* socket_suffix = "_shared_host_socket";
    int suffix_size = strlen(socket_suffix);

    char* page_name = (char*)malloc(name_size+suffix_size+1);
    memcpy(page_name, name, name_size);
    memcpy(page_name+name_size, socket_suffix, suffix_size);
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
            hexa_string[i] = cur;
        }

        key = key >> 4; // prepare the prepared key for the next iteration
    }

    hexa_string[sizeof(size_t)*2] = 0; // EOF

    return hexa_string;
}

void set_functions(shared_connection* connection, size_t flags) {
    connection->send = basic_send;
    connection->read = basic_read;

    connection->claim_send = basic_zc_claim_for_send;
    connection->publish = basic_zc_publish_for_send;

    connection->claim_read = basic_zc_claim_for_read;
    connection->drop = basic_zc_drop_for_send;
}

shared_socket* create_shared_socket(const char* name) {
    shared_socket* ret = (shared_socket*)malloc(sizeof(shared_socket));
    return ret;
}

error register_shared_socket(shared_socket* socket) {
    char* page_name = get_string_with_socket_suffix(socket->name);
    void* buffer = allocate_shm_buffer(4096, page_name); // allocate the shm socket buffer
    free(page_name);

    return 0;
}

shared_connection* accept_shared_connection(shared_socket* socket) {
    char* page_name = get_string_with_socket_suffix(socket->name);
    void* buffer = allocate_shm_buffer(4096, page_name);
    free(page_name);

    memcpy(buffer, (void*)socket, sizeof(shared_socket)); // copy all of the socket properties
    buffer += sizeof(shared_socket);

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
    free(connection_table_name);

    shared_connection* connection = (shared_connection*)malloc(sizeof(shared_connection));

    connection->job = SHARED_HOST_SERVER;
    connection->origin_socket = socket;
    connection->shared_state_table = buffer;

    set_functions(connection, socket->flags);

    *(char*)connection->shared_state_table = 1;

    return connection;
}

shared_connection* connect_shared_connection(shared_socket* socket) {
    char* page_name = get_string_with_socket_suffix(socket->name);
    void* buffer = connect_to_shm_buffer(&socket->size, page_name);
    free(page_name);

    memcpy((void*)socket, buffer, sizeof(shared_socket)); // copy all of the socket properties

    srand(time(NULL));
    size_t prepared_key = 0;

    char value_found = 1;
    while(value_found) { // search to see if the randomly generated key exists already
        prepared_key = rand();
        prepared_key += prepared_key == 0; // prepared key cannot be equal to 0
        value_found = 0;
        for (int i = 0 ; i < socket->size/sizeof(size_t) ; i++) {
            if (((size_t*)buffer)[i] == prepared_key) {
                value_found = 1;
            }
        }
    }

    for (int i = 0 ; i < socket->size/sizeof(size_t) ; i++) {
        if (atomic_compare_exchange_strong(&(((_Atomic size_t*)buffer)[i]), 0, prepared_key)) { // mark the new connection in the socket
            break;
        }
    }

    char* connection_table_name = translate_key_to_hexa_string(prepared_key);
    buffer = allocate_shm_buffer(4096, connection_table_name); // allocate the state page of the connection
    free(connection_table_name);

    // TODO: put public keys inside the state page

    shared_connection* connection = (shared_connection*)malloc(sizeof(shared_connection));

    connection->job = SHARED_HOST_CLIENT;
    connection->origin_socket = socket;
    connection->shared_state_table = buffer;

    set_functions(connection, socket->flags);

    while(*(char*)(connection->shared_state_table) == 0) {
        sleep(1); // wait for the accept action
    }

    return connection;
}
