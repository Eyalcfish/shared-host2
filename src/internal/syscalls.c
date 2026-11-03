#include "../../include/internal/syscalls.h"

#ifdef __linux__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h> // zed says this is unsed, zed is wrong

void* allocate_shm_buffer(size_t size, const char* name) {
    int fd = shm_open(name, O_RDWR | O_EXCL | O_CREAT, 0600);
    if (fd == EEXIST) {
        return 0; // name is already taken
    }

    int err = ftruncate(fd, size);
    if (err != 0) {
        return 0; // no idea what error this is
    }

    void* buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return buffer;
}

void* allocate_round_shm_buffer(size_t size, const char* name) {
    int fd = shm_open(name, O_RDWR, 0);

    return 0;
}

void* connect_to_shm_buffer(size_t* out_size, const char* name) {
    int fd = shm_open(name, O_RDWR, 0600);

    struct stat st;
    fstat(fd, &st);
    *out_size = st.st_size;

    void* buffer = mmap(NULL, *out_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return buffer;
}

void* connect_to_round_shm_buffer(size_t* out_size, const char* name) {
    return 0;
}

void free_shm_buffer_and_close_file(void* buffer, const char* name) {
    int fd = shm_open(name, O_RDWR, 0600);

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;

    munmap(buffer, size);

    shm_unlink(name);
}

void free_shm_buffer(void* buffer, const char* name) {
    int fd = shm_open(name, O_RDWR, 0600);

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;

    munmap(buffer, size);
}

#endif
