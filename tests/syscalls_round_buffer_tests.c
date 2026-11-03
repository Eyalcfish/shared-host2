#include "../include/internal/syscalls.h"
#include <unistd.h>
#include <stdio.h>
#

int main () {
    void* buffer = allocate_round_shm_buffer(4096, "test_file");
    printf("Allocated buffer %ld\n", buffer);
    int fd = fork();
    printf("My fd is %d\n", fd);
    if (fd == 0) { //child
        size_t size = 0;
        buffer = connect_to_shm_buffer(&size, "test_file");
        printf("I am the child and the size of the buffer is %ld\n", size);
        sleep(1);
        printf("i am the child and i found %d in the file\n", *(char*)buffer);
        free_round_shm_buffer(buffer, "test_file");
    }
    else {
        ((char*)buffer)[4096] = 69;
        printf("I am the parent and i am sleeping\n");
        sleep(10);
        printf("I am the parent and i stopped sleeping\n");
        free_round_shm_buffer_and_close_file(buffer, "test_file");
    }
    return 0;
}
