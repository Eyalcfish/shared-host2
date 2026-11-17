#include "../include/shared_host.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "../include/internal/read_write.h"

#define EPOCHS 10

int main () {
    int fd = fork();
    printf("My fd is %d\n", fd);
    if (fd == 0) { //child
        shared_socket* socket = create_shared_socket("test_socket");
        printf("Child: waiting for connection\n");
        shared_connection* connection = connect_shared_connection(socket);
        printf("Child: connected! to %s\n", connection->shared_state_table_name);

        char* msg = "Hello Parent!";
        shared_message message = {};
        message.origin = connection;
        message.buffer = msg;
        message.size = strlen(msg)+1;

        for(int i = 0 ; i < EPOCHS ; i ++) {
            connection->send(&message);
            StateTable* table = ((StateTable*)(connection->shared_state_table));
            printf("Client Write> %d off read: %ld write: %ld\n", i, table->client_read_offset, table->client_write_offset);

            sleep(1);

            shared_message* ret = connection->read(connection);
            // if (strcmp(ret->buffer, "Hello Child!") != 0) {
            printf("Client Read> %s %d off read: %ld write: %ld\n", (char*)ret->buffer, i, table->client_read_offset, table->client_write_offset);
                // return 0;
            // }
            free_message(ret);

        }

        sleep(5);
        close_connection(connection);
        printf("Child: closed connection!\n");
    }
    else { // parent
        shared_socket* socket = create_shared_socket("test_socket");
        socket->size = 5000; // size of connections
        printf("Parent: registering socket...\n");
        register_shared_socket(socket);
        printf("Parent: registered socket, now accepting connection...\n");
        shared_connection* connection = accept_shared_connection(socket);
        printf("Parent: connected! to %s\n", connection->shared_state_table_name);

        char* msg = "Hello Child!";
        shared_message message = {};
        message.origin = connection;
        message.buffer = msg;
        message.size = strlen(msg)+1;

        for(int i = 0 ; i < EPOCHS ; i ++) {
            shared_message* ret = connection->read(connection);

            // if (strcmp(ret->buffer, "Hello Parent!") != 0) {
            StateTable* table = ((StateTable*)(ret->origin->shared_state_table));

            printf("Server Read> %s %d off read: %ld, off write: %ld\n", (char*)ret->buffer, i, table->server_read_offset, table->server_write_offset);
                // return 0;
            // }
            free_message(ret);
            sleep(1);
            connection->send(&message);
            printf("Server Write> %d off read: %ld, off write: %ld\n", i, table->server_read_offset, table->server_write_offset);

        }

        sleep(5);
        close_connection(connection);
        printf("Parent: closed connection\n");
        unregister_socket(socket);
        printf("Parent: unregistered socket!\n");
    }
    return 0;
}
