#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
#include <stdlib.h>
#include <string.h>

// define constants within assignment 
#define BLOCK_STORE_NUM_BLOCKS 2048
#define BLOCK_SIZE_BYTES 64
#define BITMAP_START_BLOCK 1022

// struct for storage block
struct block_store {
    bitmap_t *free_blocks;    // Our checklist of used boxes
    uint8_t *blocks;          // All our storage boxes
};

// function to create a new block storage system.  
block_store_t *block_store_create() {
    // Get a new main box
    block_store_t *bs = malloc(sizeof(block_store_t));
    if (!bs) { // block not created correctly 
        return NULL;  
    }

    // calloc to make space for the blocks 
    bs->blocks = calloc(BLOCK_STORE_NUM_BLOCKS, BLOCK_SIZE_BYTES);
    if (!bs->blocks) {
        free(bs);
        return NULL;
    }

    // create the bitmap to store blocks
    bs->free_blocks = bitmap_overlay(BLOCK_STORE_NUM_BLOCKS, 
                                   bs->blocks + (BITMAP_START_BLOCK * BLOCK_SIZE_BYTES));
    
    if (!bs->free_blocks) {
        free(bs->blocks);
        free(bs);
        return NULL;
    }

    // Mark the boxes as "taken"
    bitmap_set(bs->free_blocks, BITMAP_START_BLOCK);
    bitmap_set(bs->free_blocks, BITMAP_START_BLOCK + 1);
    bitmap_set(bs->free_blocks, BITMAP_START_BLOCK + 2);
    bitmap_set(bs->free_blocks, BITMAP_START_BLOCK + 3);

    return bs;
}

// Clean up box storage
void block_store_destroy(block_store_t *const bs) {
    if (bs) {
        bitmap_destroy(bs->free_blocks);
        free(bs->blocks);
        free(bs);
    }
}

// Find an empty box and mark it as taken
size_t block_store_allocate(block_store_t *const bs) {
    if (!bs) {
        return SIZE_MAX;
    }

    // Look for an empty box
    for (size_t i = 0; i < BLOCK_STORE_NUM_BLOCKS; i++) {
        // Skip the boxes used for our checklist
        if (i >= BITMAP_START_BLOCK && i < BITMAP_START_BLOCK + 4) {
            continue;
        }
        // Found an empty box!
        if (!bitmap_test(bs->free_blocks, i)) {
            bitmap_set(bs->free_blocks, i);
            return i;
        }
    }
    return SIZE_MAX;  // No empty boxes left :(
}

// Try to get a specific box number
bool block_store_request(block_store_t *const bs, const size_t block_id) {
    if (!bs || block_id >= BLOCK_STORE_NUM_BLOCKS) {
        return false;
    }

    // check for if box isn't taken
    if (!bitmap_test(bs->free_blocks, block_id)) {
        bitmap_set(bs->free_blocks, block_id);
        return true;
    }
    return false;
}

// Mark a box as empty
void block_store_release(block_store_t *const bs, const size_t block_id) {
    if (bs && block_id < BLOCK_STORE_NUM_BLOCKS) {
        bitmap_reset(bs->free_blocks, block_id);
    }
}

// Count used boxes
size_t block_store_get_used_blocks(const block_store_t *const bs) {
    if (!bs) {
        return SIZE_MAX;
    }
    return bitmap_total_set(bs->free_blocks);
}

// Count empty boxes
size_t block_store_get_free_blocks(const block_store_t *const bs) {
    if (!bs) {
        return SIZE_MAX;
    }
    return BLOCK_STORE_NUM_BLOCKS - bitmap_total_set(bs->free_blocks);
}

// returns the size of used boxes
size_t block_store_get_total_blocks() {
    return BLOCK_STORE_NUM_BLOCKS;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer) {
    if (!bs || !buffer || block_id >= BLOCK_STORE_NUM_BLOCKS) {
        return 0;
    }

    // copies the box to the buffer 
    memcpy(buffer, bs->blocks + (block_id * BLOCK_SIZE_BYTES), BLOCK_SIZE_BYTES);
    return BLOCK_SIZE_BYTES;
}

// writes to content into the storage block 
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer) {
    if (!bs || !buffer || block_id >= BLOCK_STORE_NUM_BLOCKS) {
        return 0;
    }

    // Copy from their buffer to our box
    memcpy(bs->blocks + (block_id * BLOCK_SIZE_BYTES), buffer, BLOCK_SIZE_BYTES);
    return BLOCK_SIZE_BYTES;
}

// Save all our boxes to a file
size_t block_store_serialize(const block_store_t *const bs, const char *const filename) {
    if (!bs || !filename) {
        return 0;
    }

    // Try to save to a file
    FILE *file = fopen(filename, "wb");
    if (!file) {
        return 0;
    }

    // Write all of the boxes
    size_t written = fwrite(bs->blocks, BLOCK_SIZE_BYTES, BLOCK_STORE_NUM_BLOCKS, file);
    fclose(file);

    return written == BLOCK_STORE_NUM_BLOCKS ? (written * BLOCK_SIZE_BYTES) : 0;
}

// Load boxes from a file
block_store_t *block_store_deserialize(const char *const filename) {
    if (!filename) {
        return NULL;
    }

    // Try to open the file
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    // Make a new storage system
    block_store_t *bs = block_store_create();
    if (!bs) {
        fclose(file);
        return NULL;
    }

    // Read from file into our boxes
    size_t read = fread(bs->blocks, BLOCK_SIZE_BYTES, BLOCK_STORE_NUM_BLOCKS, file);
    fclose(file);

    // Make sure we read everything
    if (read != BLOCK_STORE_NUM_BLOCKS) {
        block_store_destroy(bs);
        return NULL;
    }

    return bs;
}