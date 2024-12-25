#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 16

typedef struct {
    unsigned char *memory;
    size_t total_size;
    size_t num_blocks;
    unsigned char *bitmap;
    size_t bitmap_size;
} McKusickKarelsAllocator;

static size_t calculate_bitmap_size(size_t num_blocks) {
    return (num_blocks + 7) / 8;
}

McKusickKarelsAllocator *allocator_create(size_t size) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); 
    size_t num_blocks = size / MIN_BLOCK_SIZE;
    size_t bitmap_size = calculate_bitmap_size(num_blocks);

    McKusickKarelsAllocator *allocator = mmap(NULL, sizeof(McKusickKarelsAllocator),
                                              PROT_READ | PROT_WRITE, 
                                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (allocator == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    allocator->memory = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (allocator->memory == MAP_FAILED) {
        perror("mmap");
        munmap(allocator, sizeof(McKusickKarelsAllocator));
        return NULL;
    }

    allocator->bitmap = mmap(NULL, bitmap_size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (allocator->bitmap == MAP_FAILED) {
        perror("mmap");
        munmap(allocator->memory, size);
        munmap(allocator, sizeof(McKusickKarelsAllocator));
        return NULL;
    }

    allocator->total_size = size;
    allocator->num_blocks = num_blocks;
    allocator->bitmap_size = bitmap_size;

    memset(allocator->bitmap, 0xFF, bitmap_size); 

    return allocator;
}

void *allocator_alloc(McKusickKarelsAllocator *allocator, size_t size) {
    size_t blocks_needed = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE;

    for (size_t i = 0; i < allocator->num_blocks - blocks_needed + 1;) {
        size_t j;
        for (j = 0; j < blocks_needed; j++) {
            if (!(allocator->bitmap[(i + j) / 8] & (1 << ((i + j) % 8)))) {
                break;
            }
        }

        if (j == blocks_needed) { 
            for (j = 0; j < blocks_needed; j++) {
                allocator->bitmap[(i + j) / 8] &= ~(1 << ((i + j) % 8)); 
            }
            return allocator->memory + i * MIN_BLOCK_SIZE;
        }
        i += j + 1; 
    }

    return NULL; 
}

void allocator_free(McKusickKarelsAllocator *allocator, void *ptr, size_t size) {
    size_t blocks_to_free = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE;
    size_t start_index = ((unsigned char *)ptr - allocator->memory) / MIN_BLOCK_SIZE;

    for (size_t i = start_index; i < start_index + blocks_to_free; i++) {
        allocator->bitmap[i / 8] |= (1 << (i % 8));
    }
}

void allocator_destroy(McKusickKarelsAllocator *allocator) {
    munmap(allocator->bitmap, allocator->bitmap_size);
    munmap(allocator->memory, allocator->total_size);
    munmap(allocator, sizeof(McKusickKarelsAllocator));
}