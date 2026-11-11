#include "../include/shared_host.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

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

            sleep(1);

            shared_message* ret = connection->read(connection);
            // if (strcmp(ret->buffer, "Hello Child!") != 0) {
                printf("Nope %s %d\n", ret->buffer, i);
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
                printf("Nope %s %d\n", ret->buffer, i);
                // return 0;
            // }
            free_message(ret);
            sleep(1);
            connection->send(&message);
        }

        sleep(5);
        close_connection(connection);
        printf("Parent: closed connection\n");
        unregister_socket(socket);
        printf("Parent: unregistered socket!\n");
    }
    return 0;
}
