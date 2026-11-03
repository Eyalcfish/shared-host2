#ifndef SYSCALLS_H
#define SYSCALLS_H
#include <stddef.h>

void* allocate_shm_buffer(size_t size, const char* name);
void* allocate_round_shm_buffer(size_t size, const char* name);

void* connect_to_shm_buffer(size_t* out_size, const char* name);
void* connect_to_round_shm_buffer(size_t* out_size, const char* name);

void free_shm_buffer_and_close_file(void* buffer, const char* name);

void free_shm_buffer(void* buffer, const char* name);

#endif
