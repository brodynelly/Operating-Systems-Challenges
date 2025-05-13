#include "dyn_array.h"
#include "bitmap.h"
#include "block_store.h"
#include "FS.h"

#define BLOCK_STORE_NUM_BLOCKS 65536    // 2^16 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 65534  // Last 2 blocks consumed by the FBM
#define BLOCK_SIZE_BITS 32768           // 2^12 BYTES per block *2^3 BITS per BYTES
#define BLOCK_SIZE_BYTES 4096           // 2^12 BYTES per block
#define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)  // 2^16 blocks of 2^12 bytes.



#define UNUSED(x) (void)(x)


// Structure definitions and function implementations for the filesystem

typedef struct {
    char name[127];          // name of file \ directory
    uint32_t inode_Index;   // index of inode into inode table
} fs_object;

// inode structure is now defined in FS.h

typedef struct {
    uint8_t inode_index;        // Which inode this descriptor refers to
    uint8_t pointer_type;       // 0 = direct, 1 = indirect, 2 = double indirect
    uint16_t block_index;       // Index within that pointer type (e.g. 0–5 for direct)
    uint16_t offset;            // Offset within the block (0–4095)
    uint8_t open_flags;         // e.g., read/write mode or access perms
} file_descriptor;


typedef struct {
    fs_object entries[31];
    uint8_t inodeNumber;
}directory_block;
\

FS_t *fs_format(const char *path) {
    if (!path || strlen(path) == 0) {
        return NULL;
    }

    FS_t * tempBlock = (FS_t *)calloc(1, sizeof(FS_t));
    if (!tempBlock) return NULL;

    tempBlock->BlockStore_fs = block_store_create(path);
    if (!tempBlock->BlockStore_fs) {
        free(tempBlock);
        return NULL;
    }


    size_t superBlock_i = block_store_allocate(tempBlock->BlockStore_fs);
    if (superBlock_i == SIZE_MAX) {

        free(tempBlock);
        return NULL;
    }
    size_t inode_table_blocks[4];
    for (int i = 0; i < 4; i++) {
        inode_table_blocks[i] = block_store_allocate(tempBlock->BlockStore_fs);
    }

    size_t root_dir_block = block_store_allocate(tempBlock->BlockStore_fs);

    inode inode_table_data[256] = {0};
    inode *root_node = &inode_table_data[0];
    root_node->vacMap = 0x00000000;
    root_node->type = 2;
    root_node->direct[0] = (uint16_t)root_dir_block;
    root_node->link_count = 2;
    root_node->inodeNumber = 0;
    root_node->size = sizeof(fs_object) * 2;

    for (int i = 0; i < 4; ++i) {
        block_store_write(tempBlock->BlockStore_fs, inode_table_blocks[i],
                          ((uint8_t *)inode_table_data) + (i * BLOCK_SIZE_BYTES));

    }

    directory_block root_dir = {0};
    strncpy(root_dir.entries[0].name, ".", 127);
    root_dir.entries[0].inode_Index = 0;
    strncpy(root_dir.entries[1].name, "..", 127);
    root_dir.entries[1].inode_Index = 0;
    root_dir.inodeNumber = 0;

    block_store_write(tempBlock->BlockStore_fs, root_dir_block, &root_dir);


    // FBM
    bitmap_t *fbm = bitmap_create(BLOCK_STORE_NUM_BLOCKS);
    if (!fbm) {

        block_store_destroy(tempBlock->BlockStore_fs);
        free(tempBlock);
        return NULL;
    }
    bitmap_set(fbm, superBlock_i);
    for (int i = 0; i < 4; ++i) bitmap_set(fbm, inode_table_blocks[i]);
    bitmap_set(fbm, root_dir_block);
    bitmap_set(fbm, 65534);
    bitmap_set(fbm, 65535);

    uint8_t fbm_buffer[BLOCK_SIZE_BYTES * 2] = {0};
    const uint8_t *fbm_data = bitmap_export(fbm);
    memcpy(fbm_buffer, fbm_data, BLOCK_SIZE_BYTES * 2);

    block_store_write(tempBlock->BlockStore_fs, 65534, fbm_buffer);
    block_store_write(tempBlock->BlockStore_fs, 65535, fbm_buffer + BLOCK_SIZE_BYTES);


    // Store the bitmap in the FS_t structure
    tempBlock->Superblock = fbm;

    // Allocate memory for the inode table
    tempBlock->inode_table = (inode *)calloc(256, sizeof(inode));
    if (!tempBlock->inode_table) {
        bitmap_destroy(fbm);
        block_store_destroy(tempBlock->BlockStore_fs);
        free(tempBlock);
        return NULL;
    }

    // Copy the inode table data to the FS_t structure
    memcpy(tempBlock->inode_table, inode_table_data, sizeof(inode) * 256);

    // Initialize file descriptor array
    tempBlock->BlockStore_fd = dyn_array_create(0, sizeof(file_descriptor), NULL);
    if (!tempBlock->BlockStore_fd) {
        free(tempBlock->inode_table);
        bitmap_destroy(fbm);
        block_store_destroy(tempBlock->BlockStore_fs);
        free(tempBlock);
        return NULL;
    }

    // Set the root inode
    tempBlock->root_inode = 0;


    return tempBlock;
}

FS_t *fs_mount(const char *path)
{
    if (!path || strlen(path) == 0) {
        return NULL;
    }

    // Create new FS_t object
    FS_t *fs = (FS_t *)calloc(1, sizeof(FS_t));
    if (!fs) {
        return NULL;
    }

    // Open the block store
    fs->BlockStore_fs = block_store_open(path);
    if (!fs->BlockStore_fs) {
        free(fs);
        return NULL;
    }


    // Allocate memory for the inode table
    fs->inode_table = (inode *)calloc(256, sizeof(inode));
    if (!fs->inode_table) {
        block_store_destroy(fs->BlockStore_fs);
        free(fs);
        return NULL;
    }

    // Load inode table from blocks 1-4
    uint8_t *inode_buffer = (uint8_t *)fs->inode_table;
    for (int i = 0; i < 4; i++) {
        size_t bytes_read = block_store_read(fs->BlockStore_fs, i + 1, inode_buffer + (i * BLOCK_SIZE_BYTES));
        if (bytes_read != BLOCK_SIZE_BYTES) {
            // Continue anyway, we'll try to work with what we have
        }
    }

    // Read the FBM from blocks 65534 and 65535
    uint8_t fbm_buffer[BLOCK_SIZE_BYTES * 2] = {0};
    size_t bytes_read = block_store_read(fs->BlockStore_fs, 65534, fbm_buffer);
    if (bytes_read != BLOCK_SIZE_BYTES) {
        // Continue anyway, we'll try to work with what we have
    }

    bytes_read = block_store_read(fs->BlockStore_fs, 65535, fbm_buffer + BLOCK_SIZE_BYTES);
    if (bytes_read != BLOCK_SIZE_BYTES) {
        // Continue anyway, we'll try to work with what we have
    }

    // Reconstruct bitmap_t using bitmap_import
    fs->Superblock = bitmap_import(BLOCK_STORE_NUM_BLOCKS, fbm_buffer);
    if (!fs->Superblock) {
        // Create a new bitmap instead
        fs->Superblock = bitmap_create(BLOCK_STORE_NUM_BLOCKS);
        if (!fs->Superblock) {
            free(fs->inode_table);
            block_store_destroy(fs->BlockStore_fs);
            free(fs);
            return NULL;
        }
    }

    // Initialize file descriptor array
    fs->BlockStore_fd = dyn_array_create(0, sizeof(file_descriptor), NULL);
    if (!fs->BlockStore_fd) {
        // Try again with a different approach
        fs->BlockStore_fd = dyn_array_create(1, sizeof(file_descriptor), NULL);
        if (!fs->BlockStore_fd) {
            if (fs->Superblock) {
                bitmap_destroy(fs->Superblock);
            }
            if (fs->inode_table) {
                free(fs->inode_table);
            }
            if (fs->BlockStore_fs) {
                block_store_destroy(fs->BlockStore_fs);
            }
            free(fs);
            return NULL;
        }
    }

    // Set the root inode
    fs->root_inode = 0;


    return fs;
}

int fs_unmount(FS_t *fs)
{
    // Check if fs is NULL
    if (!fs) {
        return -1;
    }

    // Make sure we have all the required components
    if (!fs->BlockStore_fs) {
        // Clean up whatever resources we have
        if (fs->BlockStore_fd) {
            dyn_array_destroy(fs->BlockStore_fd);
        }
        if (fs->Superblock) {
            bitmap_destroy(fs->Superblock);
        }
        if (fs->inode_table) {
            free(fs->inode_table);
        }
        free(fs);
        return -1;
    }

    // Write in-memory inode table back to blocks 1-4
    if (fs->inode_table) {
        uint8_t *inode_buffer = (uint8_t *)fs->inode_table;
        for (int i = 0; i < 4; i++) {
            size_t bytes_written = block_store_write(fs->BlockStore_fs, i + 1, inode_buffer + (i * BLOCK_SIZE_BYTES));
            if (bytes_written != BLOCK_SIZE_BYTES) {
                // Continue with cleanup even if write fails
            }
        }
    }

    // Export FBM to byte buffer
    if (fs->Superblock) {
        uint8_t fbm_buffer[BLOCK_SIZE_BYTES * 2] = {0};
        const uint8_t *fbm_data = bitmap_export(fs->Superblock);
        if (fbm_data) {
            memcpy(fbm_buffer, fbm_data, BLOCK_SIZE_BYTES * 2);

            // Write FBM to blocks 65534 and 65535
            size_t bytes_written = block_store_write(fs->BlockStore_fs, 65534, fbm_buffer);
            if (bytes_written != BLOCK_SIZE_BYTES) {
                // Continue with cleanup even if write fails
            }
            bytes_written = block_store_write(fs->BlockStore_fs, 65535, fbm_buffer + BLOCK_SIZE_BYTES);
            if (bytes_written != BLOCK_SIZE_BYTES) {
                // Continue with cleanup even if write fails
            }
        }
    }

    // Clean up resources
    if (fs->BlockStore_fd) {
        dyn_array_destroy(fs->BlockStore_fd);
    }
    if (fs->Superblock) {
        bitmap_destroy(fs->Superblock);
    }
    if (fs->inode_table) {
        free(fs->inode_table);
    }
    if (fs->BlockStore_fs) {
        block_store_destroy(fs->BlockStore_fs);
    }
    free(fs);


    return 0;
}

int fs_create(FS_t *fs, const char *path, file_t type)
{
    /* MILESTONE 2 PSEUDOCODE
    // Check parameters


    // Parse the path to get the directory and filename
    // Find the parent directory inode
    // Check if file already exists
    // Allocate a new inode
    // Initialize the new inode

    // If it's a directory, create "." and ".." entries
        // Allocate a block for the directory
        // Initialize directory block with "." and ".." entries
        // Write the directory block
        // Update the inode

    }
    // Add the new file/directory to the parent directory
    */
    UNUSED(fs);
    UNUSED(path);
    UNUSED(type);

    return 0;
}

int fs_open(FS_t *fs, const char *path)
{
     /* MILESTONE 2 PSEUDOCODE

    // Check parameters

    // Find the inode for the path
    // Get the inode
    // Check if it's a directory (can't open directories)
    // Create a new file descriptor
    // Add the file descriptor to the array
    */

    UNUSED(fs);
    UNUSED(path);
    return 0;
}

int fs_close(FS_t *fs, int fd)
{
        /* MILESTONE 2 PSEUDOCODE

    // Check parameters

    // Check if fd is valid
    // Remove the file descriptor from the array

    */
    UNUSED(fs);
    UNUSED(fd);
    return 0;
}

off_t fs_seek(FS_t *fs, int fd, off_t offset, seek_t whence)
{
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(offset);
    UNUSED(whence);
    return 0;
}

ssize_t fs_read(FS_t *fs, int fd, void *dst, size_t nbyte)
{
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(dst);
    UNUSED(nbyte);
    return 0;
}

ssize_t fs_write(FS_t *fs, int fd, const void *src, size_t nbyte)
{
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(src);
    UNUSED(nbyte);
    return 0;
}

int fs_remove(FS_t *fs, const char *path)
{
    UNUSED(fs);
    UNUSED(path);
    return 0;
}

dyn_array_t *fs_get_dir(FS_t *fs, const char *path)
{
    UNUSED(fs);
    UNUSED(path);
    return NULL;
}

int fs_move(FS_t *fs, const char *src, const char *dst)
{
    UNUSED(fs);
    UNUSED(src);
    UNUSED(dst);
    return 0;
}

int fs_link(FS_t *fs, const char *src, const char *dst)
{
    UNUSED(fs);
    UNUSED(src);
    UNUSED(dst);
    return 0;
}
