#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

typedef struct {
    void *memory;
    size_t size;
    void *free_list;
} BuddyAllocator;

BuddyAllocator* buddy_allocator_create(void *const memory, const size_t size) {
    BuddyAllocator *allocator = mmap(NULL, sizeof(BuddyAllocator), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (allocator == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    allocator->memory = memory;
    allocator->size = size;
    allocator->free_list = memory;
    *((void**)allocator->free_list) = NULL;

    return allocator;
}

void buddy_allocator_destroy(BuddyAllocator *const allocator) {
    munmap(allocator->memory, allocator->size);
    munmap(allocator, sizeof(BuddyAllocator));
}

void* buddy_allocator_alloc(BuddyAllocator *const allocator, const size_t size) {

    if (allocator->free_list == NULL || size > allocator->size) return NULL;

    void *block = allocator->free_list;
    allocator->free_list = *((void**)block);
    return block;
}

void buddy_allocator_free(BuddyAllocator *const allocator, void *const memory) {
    *((void**)memory) = allocator->free_list;
    allocator->free_list = memory;
}