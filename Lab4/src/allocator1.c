#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

typedef struct {
    void *memory;
    size_t size;
    size_t block_size;
    void *free_list;
} Allocator;

Allocator* allocator_create(void *const memory, const size_t size, const size_t block_size) {
    Allocator *allocator = mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    allocator->memory = memory;
    allocator->size = size;
    allocator->block_size = block_size;
    allocator->free_list = memory;

    char *current = (char*)memory;
    for (size_t i = 0; i < size / block_size; ++i) {
        *((void**)current) = (void*)(current + block_size);
        current += block_size;
    }

    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    munmap(allocator->memory, allocator->size);
    munmap(allocator, sizeof(Allocator));
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (allocator->free_list == NULL || size > allocator->block_size) return NULL;

    void *block = allocator->free_list;
    allocator->free_list = *((void**)block);
    return block;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    *((void**)memory) = allocator->free_list;
    allocator->free_list = memory;
}