#include "../include/shared_host.h"
#include <unistd.h>
#include <stdio.h>

int main () {
    int fd = fork();
    printf("My fd is %d\n", fd);
    if (fd == 0) { //child
        shared_socket* socket = create_shared_socket("test_socket");
        printf("Child: waiting for connection\n");
        shared_connection* connection = connect_shared_connection(socket);
        printf("Child: connected! to %s\n", connection->shared_state_table_name);
        *(char*)connection->shared_state_table = 'E';
        sleep(4);
        close_connection(connection);
        printf("Child: closed connection!\n");
    }
    else { // parent
        shared_socket* socket = create_shared_socket("test_socket");
        socket->size = 5000; // size of connections
        printf("Parent: registering socket...\b");
        register_shared_socket(socket);
        printf("Parent: registered socket, now accepting connection...\n");
        shared_connection* connection = accept_shared_connection(socket);
        printf("Parent: connected! to %s\n", connection->shared_state_table_name);
        sleep(1);
        printf("Message: %c\n", *(char*)connection->shared_state_table);
        sleep(4);
        close_connection(connection);
        printf("Parent: closed connection\n");
        unregister_socket(socket);
        printf("Parent: unregistered socket!\n");
    }
    return 0;
}
