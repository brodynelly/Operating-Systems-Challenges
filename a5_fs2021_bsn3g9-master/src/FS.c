#include "dyn_array.h"
#include "bitmap.h"
#include "block_store.h"
#include "FS.h"

#define BLOCK_STORE_NUM_BLOCKS 65536    // 2^16 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 65534  // Last 2 blocks consumed by the FBM
#define BLOCK_SIZE_BITS 32768           // 2^12 BYTES per block *2^3 BITS per BYTES
#define BLOCK_SIZE_BYTES 4096           // 2^12 BYTES per block
#define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)  // 2^16 blocks of 2^12 bytes.


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

/// Formats (and mounts) an FS file for use
/// \param fname The file to format
/// \return Mounted FS object, NULL on error
///
FS_t *fs_format(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_create(path);				// pointer to start of a large chunck of memory

        // reserve the 1st block for bitmap of inode
        size_t bitmap_ID = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("bitmap_ID = %zu\n", bitmap_ID);

        // 2rd - 5th block for inodes, 4 blocks in total
        size_t inode_start_block = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("inode_start_block = %zu\n", inode_start_block);
        for(int i = 0; i < 3; i++)
        {
            block_store_allocate(ptr_FS->BlockStore_whole);
            //			printf("all the way with block %zu\n", block_store_allocate(ptr_FS->BlockStore_whole));
        }

        // install inode block store inside the whole block store
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // the first inode is reserved for root dir
        block_store_sub_allocate(ptr_FS->BlockStore_inode);
        //		printf("first inode ID = %zu\n", block_store_sub_allocate(ptr_FS->BlockStore_inode));

        // update the root inode info.
        uint8_t root_inode_ID = 0;	// root inode is the first one in the inode table
        inode_t * root_inode = (inode_t *) calloc(1, sizeof(inode_t));
        //		printf("size of inode_t = %zu\n", sizeof(inode_t));
        root_inode->vacantFile = 0x00000000;
        root_inode->fileType = 'd';
        root_inode->inodeNumber = root_inode_ID;
        root_inode->linkCount = 1;
        //		root_inode->directPointer[0] = root_data_ID;	// not allocate date block for it until it has a sub-folder or file
        block_store_inode_write(ptr_FS->BlockStore_inode, root_inode_ID, root_inode);
        free(root_inode);

        // now allocate space for the file descriptors
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;
}



///
/// Mounts an FS object and prepares it for use
/// \param fname The file to mount

/// \return Mounted FS object, NULL on error

///
FS_t *fs_mount(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_open(path);	// get the chunck of data

        // the bitmap block should be the 1st one
        size_t bitmap_ID = 0;

        // the inode blocks start with the 2nd block, and goes around until the 5th block, 4 in total
        size_t inode_start_block = 1;

        // attach the bitmaps to their designated place
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // since file descriptors are allocated outside of the whole blocks, we can simply reallocate space for it.
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;
}




///
/// Unmounts the given object and frees all related resources
/// \param fs The FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(FS_t *fs)
{
    if(fs != NULL)
    {
        block_store_inode_destroy(fs->BlockStore_inode);

        block_store_destroy(fs->BlockStore_whole);
        block_store_fd_destroy(fs->BlockStore_fd);

        free(fs);
        return 0;
    }
    return -1;
}


// check if the input filename is valid or not
bool isValidFileName(const char *filename)
{
    if(!filename || strlen(filename) == 0 || strlen(filename) > 127 || '/'==filename[0])
    {
        return false;
    }

    return true;
}



// use str_split to decompose the input string into filenames along the path, '/' as delimiter
char** str_split(char* a_str, const char a_delim, size_t * count)
{
    if(*a_str != '/')
    {
        return NULL;
    }
    char** result    = 0;
    char* tmp        = a_str;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = '\0';

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            (*count)++;
        }
        tmp++;
    }

    result = (char**)calloc(1, sizeof(char*) * (*count));
    for(size_t i = 0; i < (*count); i++)
    {
        *(result + i) = (char*)calloc(1, 200);
    }

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            strcpy(*(result + idx++), token);
            //    *(result + idx++) = strdup(token);
            token = strtok(NULL, delim);
        }

    }
    return result;
}



///
/// Creates a new file at the specified location
///   Directories along the path that do not exist are not created
/// \param fs The FS containing the file
/// \param path Absolute path to file to create
/// \param type Type of file to create (regular/directory)
/// \return 0 on success, < 0 on failure
///
int fs_create(FS_t *fs, const char *path, file_t type)
{
    if(fs != NULL && path != NULL && strlen(path) != 0 && (type == FS_REGULAR || type == FS_DIRECTORY))
    {
        // char* copy_path = (char*)calloc(1, 65535);
        char* copy_path = (char*)calloc(1, BLOCK_STORE_NUM_BLOCKS -1); 
        if ( copy_path == NULL ) {
            printf("copy_path, line: 200; failed (returned -1 ) " ); 
            return -1; 
        }
       
        if (strlen(path) >= BLOCK_STORE_NUM_BLOCKS -1) {
            free(copy_path);

            return -1;
        }
        strcpy(copy_path, path); 
        char** tokens;		// tokens are the directory names along the path. The last one is the name for the new file or dir
        size_t count = 0;
        tokens = str_split(copy_path, '/', &count);
        free(copy_path);
        if(tokens == NULL)
        {
            return -1;
        }

        // let's check if the filenames are valid or not
        for(size_t n = 0; n < count; n++)
        {
            if(isValidFileName(*(tokens + n)) == false)
            {
                // before any return, we need to free tokens, otherwise memory leakage
                for (size_t i = 0; i < count; i++)
                {
                    free(*(tokens + i));
                }
                free(tokens);
                //printf("invalid filename\n");
                return -1;
            }
        }

        size_t parent_inode_ID = 0;	// start from the 1st inode, ie., the inode for root directory
        // first, let's find the parent dir
        size_t indicator = 0;

        // we declare parent_inode and parent_data here since it will still be used after the for loop
        directoryFile_t * parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
        inode_t * parent_inode = (inode_t *) calloc(1, sizeof(inode_t));

        for(size_t i = 0; i < count - 1; i++)
        {
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);	// read out the parent inode
            // in case file and dir has the same name
            if(parent_inode->fileType == 'd')
            {
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

                for(int j = 0; j < folder_number_entries; j++)
                {
                    if( ((parent_inode->vacantFile >> j) & 1) == 1 && strcmp((parent_data + j) -> filename, *(tokens + i)) == 0 )
                    {
                        parent_inode_ID = (parent_data + j) -> inodeNumber;
                        indicator++;
                    }
                }
            }
        }
        //		printf("indicator = %zu\n", indicator);
        //		printf("parent_inode_ID = %lu\n", parent_inode_ID);

        // read out the parent inode
        block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);
        if(indicator == count - 1 && parent_inode->fileType == 'd')
        {
            // same file or dir name in the same path is intolerable
            for(int m = 0; m < folder_number_entries; m++)
            {
                // rid out the case of existing same file or dir name
                if( ((parent_inode->vacantFile >> m) & 1) == 1)
                {
                    // before read out parent_data, we need to make sure it does exist!
                    block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                    if( strcmp((parent_data + m) -> filename, *(tokens + count - 1)) == 0 )
                    {
                        free(parent_data);
                        free(parent_inode);
                        // before any return, we need to free tokens, otherwise memory leakage
                        for (size_t i = 0; i < count; i++)
                        {
                            free(*(tokens + i));
                        }
                        free(tokens);
                        //printf("filename already exists\n");
                        return -1;
                    }
                }
            }

            // cannot declare k inside for loop, since it will be used later.
            int k = 0;
            for( ; k < folder_number_entries; k++)
            {
                if( ((parent_inode->vacantFile >> k) & 1) == 0 )
                    break;
            }

            // if k == 0, then we have to declare a new parent data block
            //			printf("k = %d\n", k);
            if(k == 0)
            {
                size_t parent_data_ID = block_store_allocate(fs->BlockStore_whole);
                //					printf("parent_data_ID = %zu\n", parent_data_ID);
                if(parent_data_ID < BLOCK_STORE_AVAIL_BLOCKS)
                {
                    parent_inode->directPointer[0] = parent_data_ID;
                }
                else
                {
                    free(parent_inode);
                    free(parent_data);
                    // before any return, we need to free tokens, otherwise memory leakage
                    for (size_t i = 0; i < count; i++)
                    {
                        free(*(tokens + i));
                    }
                    free(tokens);
                    //printf("No available blocks\n");
                    return -1;
                }
            }

            if(k < folder_number_entries)	// k == folder_number_entries means this directory is full
            {
                size_t child_inode_ID = block_store_sub_allocate(fs->BlockStore_inode);
                //printf("new child_inode_ID = %zu\n", child_inode_ID);
                // ugh, inodes are used up
                if(child_inode_ID == SIZE_MAX)
                {
                    free(parent_data);
                    free(parent_inode);
                    // before any return, we need to free tokens, otherwise memory leakage
                    for (size_t i = 0; i < count; i++)
                    {
                        free(*(tokens + i));
                    }
                    free(tokens);
                    //printf("could not allocate block for child\n");
                    return -1;
                }

                // wow, at last, we make it!
                // update the parent inode
                parent_inode->vacantFile |= (1 << k);
                // in the following cases, we should allocate parent data first:
                // 1)the parent dir is not the root dir;
                // 2)the file or dir to create is to be the 1st in the parent dir

                block_store_inode_write(fs->BlockStore_inode, parent_inode_ID, parent_inode);

                // update the parent directory file block
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                strcpy((parent_data + k)->filename, *(tokens + count - 1));
                //printf("the newly created file's name is: %s\n", (parent_data + k)->filename);
                (parent_data + k)->inodeNumber = child_inode_ID;
                block_store_write(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

                // update the newly created inode
                inode_t * child_inode = (inode_t *) calloc(1, sizeof(inode_t));
                child_inode->vacantFile = 0;
                if(type == FS_REGULAR)
                {
                    child_inode->fileType = 'r';
                }
                else if(type == FS_DIRECTORY)
                {
                    child_inode->fileType = 'd';
                }

                child_inode->inodeNumber = child_inode_ID;
                child_inode->fileSize = 0;
                child_inode->linkCount = 1;
                block_store_inode_write(fs->BlockStore_inode, child_inode_ID, child_inode);

                //printf("after creation, parent_inode->vacantFile = %d\n", parent_inode->vacantFile);



                // free the temp space
                free(parent_inode);
                free(parent_data);
                free(child_inode);
                // before any return, we need to free tokens, otherwise memory leakage
                for (size_t i = 0; i < count; i++)
                {
                    free(*(tokens + i));
                }
                free(tokens);
                return 0;
            }
        }
        // before any return, we need to free tokens, otherwise memory leakage
        for (size_t i = 0; i < count; i++)
        {
            free(*(tokens + i));
        }
        free(tokens);
        free(parent_inode);
        free(parent_data);
    }
    return -1;
}



///
/// Opens the specified file for use
///   R/W position is set to the beginning of the file (BOF)
///   Directories cannot be opened
/// \param fs The FS containing the file
/// \param path path to the requested file
/// \return file descriptor to the requested file, < 0 on error
///
int fs_open(FS_t *fs, const char *path)
{
    if(fs != NULL && path != NULL && strlen(path) != 0)
    {
         // char* copy_path = (char*)calloc(1, 65535);
         char* copy_path = (char*)calloc(1, BLOCK_STORE_NUM_BLOCKS -1); 
         if ( copy_path == NULL ) {
             printf("copy_path, line: 200; failed (returned -1 ) " ); 
             return -1; 
         }
        
         if (strlen(path) >= BLOCK_STORE_NUM_BLOCKS -1) {
             free(copy_path);
 
             return -1;
         }
         strcpy(copy_path, path); 
        char** tokens;		// tokens are the directory names along the path. The last one is the name for the new file or dir
        size_t count = 0;
        tokens = str_split(copy_path, '/', &count);
        free(copy_path);
        if(tokens == NULL)
        {
            return -1;
        }

        // let's check if the filenames are valid or not
        for(size_t n = 0; n < count; n++)
        {
            if(isValidFileName(*(tokens + n)) == false)
            {
                // before any return, we need to free tokens, otherwise memory leakage
                for (size_t i = 0; i < count; i++)
                {
                    free(*(tokens + i));
                }
                free(tokens);
                return -1;
            }
        }

        size_t parent_inode_ID = 0;	// start from the 1st inode, ie., the inode for root directory
        // first, let's find the parent dir
        size_t indicator = 0;

        inode_t * parent_inode = (inode_t *) calloc(1, sizeof(inode_t));
        directoryFile_t * parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);

        // locate the file
        for(size_t i = 0; i < count; i++)
        {
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);	// read out the parent inode
            if(parent_inode->fileType == 'd')
            {
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                //printf("parent_inode->vacantFile = %d\n", parent_inode->vacantFile);
                for(int j = 0; j < folder_number_entries; j++)
                {
                    //printf("(parent_data + j) -> filename = %s\n", (parent_data + j) -> filename);
                    if( ((parent_inode->vacantFile >> j) & 1) == 1 && strcmp((parent_data + j) -> filename, *(tokens + i)) == 0 )
                    {
                        parent_inode_ID = (parent_data + j) -> inodeNumber;
                        indicator++;
                    }
                }
            }
        }
        free(parent_data);
        free(parent_inode);
        //printf("indicator = %zu\n", indicator);
        //printf("count = %zu\n", count);
        // now let's open the file
        if(indicator == count)
        {
            size_t fd_ID = block_store_sub_allocate(fs->BlockStore_fd);
            //printf("fd_ID = %zu\n", fd_ID);
            // it could be possible that fd runs out
            if(fd_ID < number_fd)
            {
                size_t file_inode_ID = parent_inode_ID;
                inode_t * file_inode = (inode_t *) calloc(1, sizeof(inode_t));
                block_store_inode_read(fs->BlockStore_inode, file_inode_ID, file_inode);	// read out the file inode

                // it's too bad if file to be opened is a dir
                if(file_inode->fileType == 'd')
                {
                    free(file_inode);
                    // before any return, we need to free tokens, otherwise memory leakage
                    for (size_t i = 0; i < count; i++)
                    {
                        free(*(tokens + i));
                    }
                    free(tokens);
                    return -1;
                }

                // assign a file descriptor ID to the open behavior
                fileDescriptor_t * fd = (fileDescriptor_t *)calloc(1, sizeof(fileDescriptor_t));
                fd->inodeNum = file_inode_ID;
                fd->usage = 1;
                fd->locate_order = 0; // R/W position is set to the beginning of the file (BOF)
                fd->locate_offset = 0;
                block_store_fd_write(fs->BlockStore_fd, fd_ID, fd);

                free(file_inode);
                free(fd);
                // before any return, we need to free tokens, otherwise memory leakage
                for (size_t i = 0; i < count; i++)
                {
                    free(*(tokens + i));
                }
                free(tokens);
                return fd_ID;
            }
        }
        // before any return, we need to free tokens, otherwise memory leakage
        for (size_t i = 0; i < count; i++)
        {
            free(*(tokens + i));
        }
        free(tokens);
    }
    return -1;
}


///
/// Closes the given file descriptor
/// \param fs The FS containing the file
/// \param fd The file to close
/// \return 0 on success, < 0 on failure
///
int fs_close(FS_t *fs, int fd)
{
    if(fs != NULL && fd >=0 && fd < number_fd)
    {
        // first, make sure this fd is in use
        if(block_store_sub_test(fs->BlockStore_fd, fd))
        {
            block_store_sub_release(fs->BlockStore_fd, fd);
            return 0;
        }
    }
    return -1;
}



///
/// Populates a dyn_array with information about the files in a directory
///   Array contains up to 31 file_record_t structures
/// \param fs The FS containing the file
/// \param path Absolute path to the directory to inspect
/// \return dyn_array of file records, NULL on error
///
dyn_array_t *fs_get_dir(FS_t *fs, const char *path)
{
    if(fs != NULL && path != NULL && strlen(path) != 0)
    {
        // char* copy_path = (char*)calloc(1, 65535);
        char* copy_path = (char*)calloc(1, BLOCK_STORE_NUM_BLOCKS -1); 
        if ( copy_path == NULL ) {
            printf("copy_path, line: 200; failed (returned -1 ) " ); 
            return NULL; 
        }
       
        if (strlen(path) >= BLOCK_STORE_NUM_BLOCKS -1) {
            free(copy_path);

            return NULL;
        }
        strcpy(copy_path, path); 
        char** tokens;		// tokens are the directory names along the path. The last one is the name for the new file or dir
        size_t count = 0;
        tokens = str_split(copy_path, '/', &count);
        free(copy_path);

        if(strlen(*tokens) == 0)
        {
            // a spcial case: only a slash, no dir names
            count -= 1;
        }
        else
        {
            for(size_t n = 0; n < count; n++)
            {
                if(isValidFileName(*(tokens + n)) == false)
                {
                    // before any return, we need to free tokens, otherwise memory leakage
                    for (size_t i = 0; i < count; i++)
                    {
                        free(*(tokens + i));
                    }
                    free(tokens);
                    return NULL;
                }
            }
        }

        // search along the path and find the deepest dir
        size_t parent_inode_ID = 0;	// start from the 1st inode, ie., the inode for root directory
        // first, let's find the parent dir
        size_t indicator = 0;

        inode_t * parent_inode = (inode_t *) calloc(1, sizeof(inode_t));
        directoryFile_t * parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
        for(size_t i = 0; i < count; i++)
        {
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);	// read out the parent inode
            // in case file and dir has the same name. But from the test cases we can see, this case would not happen
            if(parent_inode->fileType == 'd')
            {
                block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);
                for(int j = 0; j < folder_number_entries; j++)
                {
                    if( ((parent_inode->vacantFile >> j) & 1) == 1 && strcmp((parent_data + j) -> filename, *(tokens + i)) == 0 )
                    {
                        parent_inode_ID = (parent_data + j) -> inodeNumber;
                        indicator++;
                    }
                }
            }
        }
        free(parent_data);
        free(parent_inode);

        // now let's enumerate the files/dir in it
        if(indicator == count)
        {
            inode_t * dir_inode = (inode_t *) calloc(1, sizeof(inode_t));
            block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, dir_inode);	// read out the file inode
            if(dir_inode->fileType == 'd')
            {
                // prepare the data to be read out
                directoryFile_t * dir_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
                block_store_read(fs->BlockStore_whole, dir_inode->directPointer[0], dir_data);

                // prepare the dyn_array to hold the data
                dyn_array_t * dynArray = dyn_array_create(folder_number_entries, sizeof(file_record_t), NULL);

                for(int j = 0; j < folder_number_entries; j++)
                {
                    if( ((dir_inode->vacantFile >> j) & 1) == 1 )
                    {
                        file_record_t* fileRec = (file_record_t *)calloc(1, sizeof(file_record_t));
                        strcpy(fileRec->name, (dir_data + j) -> filename);

                        // to know fileType of the member in this dir, we have to refer to its inode
                        inode_t * member_inode = (inode_t *) calloc(1, sizeof(inode_t));
                        block_store_inode_read(fs->BlockStore_inode, (dir_data + j) -> inodeNumber, member_inode);
                        if(member_inode->fileType == 'd')
                        {
                            fileRec->type = FS_DIRECTORY;
                        }
                        else if(member_inode->fileType == 'f')
                        {
                            fileRec->type = FS_REGULAR;
                        }

                        // now insert the file record into the dyn_array
                        dyn_array_push_front(dynArray, fileRec);
                        free(fileRec);
                        free(member_inode);
                    }
                }
                free(dir_data);
                free(dir_inode);
                // before any return, we need to free tokens, otherwise memory leakage
                if(strlen(*tokens) == 0)
                {
                    // a spcial case: only a slash, no dir names
                    count += 1;
                }
                for (size_t i = 0; i < count; i++)
                {
                    free(*(tokens + i));
                }
                free(tokens);
                return(dynArray);
            }
            free(dir_inode);
        }
        // before any return, we need to free tokens, otherwise memory leakage
        if(strlen(*tokens) == 0)
        {
            // a spcial case: only a slash, no dir names
            count += 1;
        }
        for (size_t i = 0; i < count; i++)
        {
            free(*(tokens + i));
        }
        free(tokens);
    }
    return NULL;
}
off_t fs_seek(FS_t *fs, int fd, off_t offset, seek_t whence) 
{
    if(fs == NULL){
        return -1;
    }
    //make sure we have valid fd
    if(!block_store_sub_test(fs->BlockStore_fd, fd))
    {
        return -1;
    }
    //pull down file descriptor based on num given
    fileDescriptor_t* fileDescr = calloc(1,sizeof(fileDescriptor_t));
    if(fileDescr == NULL ) {
        return -1;
    }
    size_t fd_bytes_read = block_store_fd_read(fs->BlockStore_fd,fd,fileDescr);
    if(fd_bytes_read != sizeof(fileDescriptor_t) || fileDescr->inodeNum == 0) {
        //if we read less than the # of bytes or the inode # is 0 (which should never happen), then we must have been given invalid fd.
        free(fileDescr);
        return -1;
    }
    inode_t* fileInode = calloc(1, sizeof(inode_t));
    if(fileInode == NULL) {
        free(fileDescr);
        return -1;
    }
    //get inode we are writing to.
    block_store_inode_read(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
    if(offset == 0 && whence == FS_SEEK_CUR) {
        //if no offset, just get current offset to return.
        off_t current = fileDescr->locate_order*BLOCK_SIZE_BYTES + fileDescr->locate_offset;
        free(fileDescr);
        free(fileInode);
        return current;
    }
    if(whence == FS_SEEK_CUR) {
        if(offset < 0) {
            //we need to make sure we are not going before BOF if this is the case.
            off_t pos_offset = offset * -1;
            uint16_t numblocks = pos_offset / BLOCK_SIZE_BYTES;
            if(fileDescr->usage == 1) {
                if(numblocks - fileDescr->locate_order < 0) {
                    //going past beginning so set to 0 for each
                    fileDescr->usage = 1;
                    fileDescr->locate_order = 0;
                    fileDescr->locate_offset = 0;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(fileInode);
                    return 0;
                }
                else if(fileDescr->locate_order == 0 && fileDescr->locate_offset - (pos_offset % BLOCK_SIZE_BYTES) < 0) {
                    //going past beginning
                    fileDescr->usage = 1;
                    fileDescr->locate_order = 0;
                    fileDescr->locate_offset = 0;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(fileInode);
                    return 0;
                }
            }
            else if(fileDescr->usage == 2) {
                if(numblocks - (fileDescr->locate_order + 6) < 0) {
                    fileDescr->usage = 1;
                    fileDescr->locate_order = 0;
                    fileDescr->locate_offset = 0;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(fileInode);
                    return 0;
                }
            }
            else {
                if(numblocks - (fileDescr->locate_order + 6) < 0) {
                    fileDescr->usage = 1;
                    fileDescr->locate_order = 0;
                    fileDescr->locate_offset = 0;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(fileInode);
                    return 0;
                }
            }
            fileDescr->locate_order -= numblocks;
            fileDescr->locate_offset -= (pos_offset % BLOCK_SIZE_BYTES);
            if(fileDescr->locate_order > UINT16_MAX - BLOCK_SIZE_BYTES && fileDescr->usage > 1) {
                if(fileDescr->usage == 2) {
                    fileDescr->usage = 1;
                }
                else {
                    fileDescr->usage = 2;
                }
                fileDescr->locate_order = 2048;
            }
            //if we wrap around due to negative overflow, then...
            if(fileDescr->locate_offset > UINT16_MAX - BLOCK_SIZE_BYTES) {
                //smaller than a block, so decrease block count by one and set to end of that block.
                uint16_t offset_to_subtract = fileDescr->locate_offset * -1;
                fileDescr->locate_order--;
                fileDescr->locate_offset = BLOCK_SIZE_BYTES - offset_to_subtract;
            }
        }
        else {
            //going forward in file from current pos.
            uint16_t numblocks = offset / BLOCK_SIZE_BYTES;
            fileDescr->locate_order += numblocks;
            fileDescr->locate_offset += (offset % BLOCK_SIZE_BYTES);
            if(fileDescr->locate_order > 2048 && fileDescr->usage < 4) {
                if(fileDescr->usage == 2) {
                    fileDescr->usage = 4;
                    fileDescr->locate_order -= 2048 + 6;
                }
                else {
                    fileDescr->usage = 2;
                    fileDescr->locate_order -= 6;
                }
                if(fileDescr->locate_order > 2048 + 6  && fileDescr->usage == 2) {
                    fileDescr->usage = 4;
                    fileDescr->locate_order -= 2048;
                }
            }
            if(fileDescr->locate_offset > BLOCK_SIZE_BYTES) {
                fileDescr->locate_order++;
                fileDescr->locate_offset -= BLOCK_SIZE_BYTES;
            }
        }
    }
    else if(whence == FS_SEEK_SET) {
        if(offset < 0) {
            //can't have negative offset when setting, as we will go before BOF.
            free(fileDescr);
            free(fileInode);
            return 0;
        }
        fileDescr->locate_offset = offset%BLOCK_SIZE_BYTES;
        if(offset / BLOCK_SIZE_BYTES < 6) {
            //we are within direct pointer
            fileDescr->usage = 1;
            fileDescr->locate_order = offset/BLOCK_SIZE_BYTES;
        }
        else if(offset / BLOCK_SIZE_BYTES < 2048 + 6) {
            //within indirect
            fileDescr->usage = 2;
            fileDescr->locate_order = (offset/BLOCK_SIZE_BYTES) - 6;
        }
        else {
            //within double indirect
            fileDescr->usage = 4;
            fileDescr->locate_order = (offset/BLOCK_SIZE_BYTES) - 6;

        }
    }
    else if(whence == FS_SEEK_END) {
        if(offset > 0) {
            //can't go past eof
            free(fileDescr);
            free(fileInode);
            return 0;
        }
        off_t pos_offset = offset * -1;
        uint16_t numblocks = pos_offset / BLOCK_SIZE_BYTES;
        fileDescr->locate_order -= numblocks;
        fileDescr->locate_offset -= (pos_offset % BLOCK_SIZE_BYTES);
        if(fileDescr->locate_order > UINT16_MAX - BLOCK_SIZE_BYTES && fileDescr->usage > 1) {
            if(fileDescr->usage == 2) {
                fileDescr->usage = 1;
            }
            else {
                fileDescr->usage = 2;
            }
            fileDescr->locate_order = 2048;
        }
        //if we wrap around due to negative overflow, then...
        if(fileDescr->locate_offset > UINT16_MAX - BLOCK_SIZE_BYTES) {
            //smaller than a block, so decrease block count by one and set to end of that block.
            uint16_t offset_to_subtract = fileDescr->locate_offset * -1;
            fileDescr->locate_order--;
            fileDescr->locate_offset = BLOCK_SIZE_BYTES - offset_to_subtract;
        }
    }
    else {
        //invalid whence, free & return error.
        free(fileDescr);
        free(fileInode);
        return -1;
    }
    //need to see if we went past EOF so set the offset to -1 to the most recent writable bit
    if(fileDescr->usage == 4 && (fileDescr->locate_order+6+2048) > 65000) {
        //very close to EOF, so decreement by 1.
        fileDescr->locate_order--;
        fileDescr->locate_offset = BLOCK_SIZE_BYTES - 1;
    }
    //write back file descr when done
    off_t from_beginning = 0;
    if(fileDescr->usage == 1){
        from_beginning = fileDescr->locate_order*BLOCK_SIZE_BYTES + fileDescr->locate_offset;
    }
    else if(fileDescr->usage == 2) {
        from_beginning = (fileDescr->locate_order+6)*BLOCK_SIZE_BYTES + fileDescr->locate_offset;
    }
    else {
        from_beginning = (fileDescr->locate_order+6+2048)*BLOCK_SIZE_BYTES + fileDescr->locate_offset;
    }
    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
    free(fileDescr);
    free(fileInode);
    return from_beginning;
}
ssize_t fs_read(FS_t *fs, int fd, void *dst, size_t nbyte)
{
    // Check for valid parameters
    if (fs == NULL || fd < 0 || fd >= number_fd || dst == NULL || !nbyte ) {
        return -1;
    }

    if ( nbyte == 0 ) {
        // empty read byte req
        return 0; 
    }

    // Check if the file descriptor is in use
    if (!block_store_sub_test(fs->BlockStore_fd, fd)) {
        return -1;
    }

    // Get the file descriptor
    fileDescriptor_t *file_desc = (fileDescriptor_t *)calloc(1, sizeof(fileDescriptor_t));
    if (file_desc == NULL) {
        return -1;
    }
    block_store_fd_read(fs->BlockStore_fd, fd, file_desc);

    // Get the inode for this file
    inode_t *inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (inode == NULL) {
        free(file_desc);
        return -1;
    }
    block_store_inode_read(fs->BlockStore_inode, file_desc->inodeNum, inode);

    // Calculate current position
    off_t current_pos = 0;
    if (file_desc->usage == 1) { // Direct pointer
        current_pos = file_desc->locate_order * BLOCK_SIZE_BYTES + file_desc->locate_offset;
    } else if (file_desc->usage == 2) { // Indirect pointer
        current_pos = 6 * BLOCK_SIZE_BYTES + file_desc->locate_order * BLOCK_SIZE_BYTES + file_desc->locate_offset;
    } else if (file_desc->usage == 4) { // Double indirect pointer
        current_pos = 6 * BLOCK_SIZE_BYTES + 1024 * BLOCK_SIZE_BYTES +
                      file_desc->locate_order * BLOCK_SIZE_BYTES + file_desc->locate_offset;
    }

    // Limit read to file size
    ssize_t bytes_to_read = nbyte;
    if (current_pos + bytes_to_read > inode->fileSize) {
        bytes_to_read = inode->fileSize - current_pos;
        if (bytes_to_read <= 0) {
            free(file_desc);
            free(inode);
            return 0; // At or past EOF, nothing to read
        }
    }

    // Prepare for reading
    ssize_t bytes_read = 0;
    uint8_t *dst_ptr = (uint8_t *)dst;
    size_t entries_per_indirect = BLOCK_SIZE_BYTES / sizeof(uint16_t);
    printf( "size of entries_per_indirect %zu", entries_per_indirect); 


    // Read data from blocks
    while (bytes_read < bytes_to_read) {
        uint16_t block_id = 0;

        // Determine which block to read from
        if (file_desc->usage == 1) { // Direct pointer
            if (file_desc->locate_order >= 6) {
                break; // Beyond direct pointers
            }
            block_id = inode->directPointer[file_desc->locate_order];

        } else if (file_desc->usage == 2) { // Indirect pointer
            uint16_t *indirect_data = (uint16_t *)calloc(1, BLOCK_SIZE_BYTES);
            if (indirect_data == NULL) {
                free(file_desc);
                free(inode);
                return -1;
            }
            block_store_read(fs->BlockStore_whole, inode->indirectPointer[0], indirect_data);

 \
             // Check if block exists, allocate if needed
            if (indirect_data[file_desc->locate_order] == 0) {
                size_t new_block = block_store_allocate(fs->BlockStore_whole);
                if (new_block == SIZE_MAX) {
                    free(indirect_data);
                    free(file_desc);
                    free(inode);
                    return -1;
                }
                indirect_data[file_desc->locate_order] = new_block;
                block_store_write(fs->BlockStore_whole, inode->indirectPointer[0], indirect_data);
            }
            block_id = indirect_data[file_desc->locate_order];
            free(indirect_data);

            
            // FIX: Read correct block size
            // block_store_read(fs->BlockStore_whole, inode->indirectPointer[0], indirect_data);


        } else {
            // Double indirect not implemented
            break;
        }

        // If block doesn't exist, we're done
        if (block_id == 0) {
            break;
        }

        // Read the block
        uint8_t *block_data = (uint8_t *)calloc(1, BLOCK_SIZE_BYTES);
        block_store_read(fs->BlockStore_whole, block_id, block_data);

        // Calculate how much to read from this block
        size_t block_bytes_to_read = BLOCK_SIZE_BYTES - file_desc->locate_offset;
        if (bytes_read + block_bytes_to_read > bytes_to_read) {
            block_bytes_to_read = bytes_to_read - bytes_read;
        }

        // Copy data from block to destination
        memcpy(dst_ptr + bytes_read, block_data + file_desc->locate_offset, block_bytes_to_read);
        bytes_read += block_bytes_to_read;

        // Update file position
        file_desc->locate_offset += block_bytes_to_read;
        if (file_desc->locate_offset >= BLOCK_SIZE_BYTES) {
            file_desc->locate_order++;
            file_desc->locate_offset = 0;

            // Check if we need to move to indirect pointers
            if (file_desc->usage == 1 && file_desc->locate_order >= 6) {
                file_desc->usage = 2;
                file_desc->locate_order = 0;
            }
        }

        free(block_data);
    }

    // Update file descriptor
    block_store_fd_write(fs->BlockStore_fd, fd, file_desc);

    // Clean up
    free(file_desc);
    free(inode);

    return bytes_read;
}

ssize_t fs_write(FS_t *fs, int fd, const void *src, size_t nbyte)
{
    //PSEUDOCODE:
    /*
    first, error check all parameters to ensure all not null or invalid
    next, check to make sure the file descriptor is valid, and that it points to a valid file
    At this point, writing can commence. We start at the start at the given block/offset indicated by the fd.
    We write nbytes into the given block starting at the given offset using the given src. If we run to the end of a block, we ask for another block, update the inode and continue writing.
    If we run out of blocks return an error, we stop and return what we have. We finally return what was how many bytes were written.
    */
    //error check parameters
    if(fs == NULL || src == NULL) {
        return -1;
    }
    //check and make sure the fd is valid
    if(!block_store_sub_test(fs->BlockStore_fd, fd))
    {
        return -1;
    }
    //pull down file descriptor based on num given
    fileDescriptor_t* fileDescr = calloc(1,sizeof(fileDescriptor_t));
    if(fileDescr == NULL ) {
        return -1;
    }
    size_t fd_bytes_read = block_store_fd_read(fs->BlockStore_fd,fd,fileDescr);
    if(fd_bytes_read != sizeof(fileDescriptor_t) || fileDescr->inodeNum == 0) {
        //if we read less than the # of bytes or the inode # is 0 (which should never happen), then we must have been given invalid fd.
        free(fileDescr);
        return -1;
    }
    inode_t* fileInode = calloc(1, sizeof(inode_t));
    if(fileInode == NULL) {
        free(fileDescr);
        return -1;
    }
    //get inode we are writing to.
    block_store_inode_read(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);

    //array for double indirect, holding pointers to indirect block & indirect array
    //uint16_t doubleIndirectPtrArr[2048] = {0};
    uint16_t indirectPtrArr[2048] = {0};
    if(nbyte == 0) {
        //if we aren't writing at all, just return at this point. Don't need to update anything but overwrite inode just in case.
        block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
        free(fileInode);
        free(fileDescr);
        return 0;
    }
    //get temp buffer that allows each block's data to be written individually
    void* tempBuffer = calloc(1, BLOCK_SIZE_BYTES);
    //pointer blocks setup now if needed, now we just loop through data and write to it
    //at this point, we passed all error checking, so if we allocate a block and it fails, we are out of space, so return bytes written.
    size_t bytes_written = 0;
    for(bytes_written = 0; bytes_written != nbyte;) {
        //if offset is already advanced into current block, then we might not need another block
        if(fileDescr->locate_offset == 0) {
            //need new block allocated since offset is 0
            size_t block_num = 0;
            if(fileDescr->usage ==1) {
                //we know we are still using direct ptr blocks, so find open block.
                int count;
                for(count = 0; count < 6; count++) {
                    if(fileInode->directPointer[count] == 0) {
                        break;
                    }
                }
                //found space in direct ptr
                block_num = block_store_allocate(fs->BlockStore_whole);
                fileInode->directPointer[count] = block_num;
                if (block_num == SIZE_MAX) {
                    //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                    //write updated inode back to bs
                    block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                    free(fileInode);
                    fileInode = NULL;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(tempBuffer);
                    return bytes_written;
                }

            }
            else if(fileDescr->usage == 2) {
                //using indirect
                block_store_read(fs->BlockStore_whole,fileInode->indirectPointer[0],indirectPtrArr);
                int indirect_block_array_num = 0;
                for(indirect_block_array_num = 0; indirect_block_array_num < 2048; indirect_block_array_num++) {
                    //look for open spot in indirectArr
                    if(indirectPtrArr[indirect_block_array_num] == 0) {
                        break;
                    }
                }
                    block_num = block_store_allocate(fs->BlockStore_whole);
                    if(block_num == SIZE_MAX) {
                        //write updated inode back to bs
                        //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                        block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                        free(fileInode);
                        fileInode = NULL;
                        block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                        free(fileDescr);
                        free(tempBuffer);
                        return bytes_written;
                    }
                    //set pointer in array, write back to block
                    indirectPtrArr[indirect_block_array_num] = block_num;
                    block_store_write(fs->BlockStore_whole,fileInode->indirectPointer[0],indirectPtrArr);
            }
            else {
                //writing to double indirect
                uint16_t doubleIndirectArr[2048];
                block_store_read(fs->BlockStore_whole,fileInode->doubleIndirectPointer,doubleIndirectArr);
                int double_indirect_block_array_num = 0;
                for(double_indirect_block_array_num = 0; double_indirect_block_array_num < 2048; double_indirect_block_array_num++) {
                    //look for edge occupied spot in double_indirectArr
                    if(doubleIndirectArr[double_indirect_block_array_num] != 0 && doubleIndirectArr[double_indirect_block_array_num+1] == 0) {
                        break;
                    }
                }
                //now read given indirect at block num
                block_store_read(fs->BlockStore_whole,doubleIndirectArr[double_indirect_block_array_num],indirectPtrArr);
                int indirect_block_array_num = 0;
                for(indirect_block_array_num = 0; indirect_block_array_num < 2048; indirect_block_array_num++) {
                    //look for open spot in indirectArr
                    if(indirectPtrArr[indirect_block_array_num] == 0) {
                        break;
                    }
                }
                if(indirect_block_array_num == 2048) {
                    //out of space in given block, so let's allocate space in the next
                    size_t next_block = block_store_allocate(fs->BlockStore_whole);
                    uint16_t* blank_indirect = calloc(2048,sizeof(uint16_t));
                    if(next_block == SIZE_MAX) {
                        //write updated inode back to bs
                        //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                        block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                        free(fileInode);
                        fileInode = NULL;
                        block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                        free(fileDescr);
                        free(tempBuffer);
                        return bytes_written;
                    }
                    doubleIndirectArr[double_indirect_block_array_num+1] = next_block;
                    //write back changes
                    block_store_write(fs->BlockStore_whole,fileInode->doubleIndirectPointer,doubleIndirectArr);
                    block_store_write(fs->BlockStore_whole,next_block,blank_indirect);
                    free(blank_indirect);
                    //restart this loop iteration
                    continue;
                }
                //we found space, so lets allocate it.
                block_num = block_store_allocate(fs->BlockStore_whole);
                if(block_num == SIZE_MAX) {
                    //write updated inode back to bs
                    //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                    block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                    free(fileInode);
                    fileInode = NULL;
                    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                    free(fileDescr);
                    free(tempBuffer);
                    return bytes_written;
                }
                //set pointer in array, write back to block
                indirectPtrArr[indirect_block_array_num] = block_num;
                block_store_write(fs->BlockStore_whole,doubleIndirectArr[double_indirect_block_array_num],indirectPtrArr);
            }
            //actually write to block
            if(nbyte - bytes_written < BLOCK_SIZE_BYTES) {
                //writing to less than block
                memcpy(tempBuffer,(src+bytes_written),nbyte-bytes_written);
                //now update fd
                fileDescr->locate_offset += nbyte-bytes_written;
                bytes_written += nbyte-bytes_written;
            }
            else {
                //writing to whole block
                memcpy(tempBuffer,(src+bytes_written),BLOCK_SIZE_BYTES);
                fileDescr->locate_order++;
                bytes_written += BLOCK_SIZE_BYTES;
            }
            //actually physically write to given block
            block_store_write(fs->BlockStore_whole, block_num, tempBuffer);
        }
        else {
            //lets just write as much as we can to this existing block based on offset
            void* current_block = calloc(1,BLOCK_SIZE_BYTES);
            uint16_t block_index = 0;
            uint16_t block_id = 0;
            block_index = fileDescr->locate_order;
            if(fileDescr->usage == 1) {
                //using direct
                if(fileInode->directPointer[block_index] == 0) {
                    //can't read empty block, error somewhere, so indicate that.
                    free(fileDescr);
                    free(fileInode);
                    free(current_block);
                    return -1;
                }
                block_id = fileInode->directPointer[block_index];
                block_store_read(fs->BlockStore_whole,fileInode->directPointer[block_index],current_block);
            }
            else if(fileDescr->usage == 2) {
                if(fileInode->indirectPointer[0] == 0) {
                    //can't read empty block, error somewhere, so indicate that.
                    free(fileDescr);
                    free(fileInode);
                    free(current_block);
                    return -1;
                }
                uint16_t indirectArr[2048] = {0};
                block_store_read(fs->BlockStore_whole,fileInode->indirectPointer[0],indirectArr);
                block_id = indirectArr[block_index];
                block_store_read(fs->BlockStore_whole,indirectArr[block_index],current_block);
            }
            else {
                //double indirect
                if(fileInode->doubleIndirectPointer == 0) {
                    //can't read empty block, error somewhere, so indicate that.
                    free(fileDescr);
                    free(fileInode);
                    free(current_block);
                    return -1;
                }
                uint16_t doubleIndirectArr[2048] = {0};
                block_store_read(fs->BlockStore_whole,fileInode->indirectPointer[0],doubleIndirectArr);
                uint16_t indirectArr[2048] = {0};
                block_store_read(fs->BlockStore_whole,block_index / 2048,indirectArr);
                block_id = indirectArr[block_index % 2048];
                block_store_read(fs->BlockStore_whole,indirectArr[block_index % 2048], current_block);
            }
            //we have current block, lets just write data all the way to the end of it.
            uint16_t loc = nbyte - bytes_written;
            if(loc < (BLOCK_SIZE_BYTES - fileDescr->locate_offset)) {
                //staying within current block this write.
                memcpy( (current_block + fileDescr->locate_offset),(bytes_written + src),nbyte-bytes_written);
                fileDescr->locate_offset = nbyte-bytes_written;
                bytes_written += nbyte-bytes_written;
            }
            else {
                //writing to end of block
                size_t bytes_to_write_this_iter = BLOCK_SIZE_BYTES - fileDescr->locate_offset;
                memcpy( (current_block + fileDescr->locate_offset),(bytes_written + src),bytes_to_write_this_iter);
                bytes_written += bytes_to_write_this_iter;
                //now need to increment locate_order & set locate_offset to 0
                fileDescr->locate_order++;
                fileDescr->locate_offset = 0;
            }
            //write back block
            block_store_write(fs->BlockStore_whole,block_id,current_block);
            free(current_block);
        }
        if(fileDescr->locate_order == 6 && fileDescr->usage == 1) {
            //need to go up to indirect blocks, so let's allocate space for indirect
            uint16_t blank_indirect[2048] = {0};
            size_t indirect_block = block_store_allocate(fs->BlockStore_whole);
            if(indirect_block == SIZE_MAX) {
                //write updated inode back to bs
                //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                free(fileInode);
                fileInode = NULL;
                block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                free(fileDescr);
                free(tempBuffer);
                return bytes_written;
            }
            fileInode->indirectPointer[0] = indirect_block;
            block_store_write(fs->BlockStore_whole,indirect_block,blank_indirect);
            fileDescr->locate_order = 0;
            fileDescr->usage = 2;
        }
        else if(fileDescr->locate_order == 2048 && fileDescr->usage == 2) {
            //go up to double indirect
            //need to go up to indirect blocks, so let's allocate space for indirect
            uint16_t blank_double_indirect[2048] = {0};
            size_t double_indirect_block = block_store_allocate(fs->BlockStore_whole);
            if(double_indirect_block == SIZE_MAX) {
                //write updated inode back to bs
                //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                free(fileInode);
                fileInode = NULL;
                block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                free(fileDescr);
                free(tempBuffer);
                return bytes_written;
            }
            fileInode->doubleIndirectPointer = double_indirect_block;
            //allocate space for indirect block as well
            uint16_t blank_indirect[2048] = {0};
            size_t indirect_block = block_store_allocate(fs->BlockStore_whole);
            if(indirect_block == SIZE_MAX) {
                //write updated inode back to bs
                //uh oh error, ran out of blocks, so free everything, and write back what was done so far.
                block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
                free(fileInode);
                fileInode = NULL;
                block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
                free(fileDescr);
                free(tempBuffer);
                return bytes_written;
            }
            blank_double_indirect[0] = indirect_block;
            block_store_write(fs->BlockStore_whole,indirect_block,blank_indirect);
            block_store_write(fs->BlockStore_whole,double_indirect_block,blank_double_indirect);
            fileDescr->locate_order = 0;
            fileDescr->usage = 4;
        }
    }

    //wrote everything back, so we can update everything and return how many bytes we wrote.
    //write updated inode back to bs
    block_store_inode_write(fs->BlockStore_inode,fileDescr->inodeNum,fileInode);
    free(fileInode);
    fileInode = NULL;
    //fileDescr->locate_order = numOfBlocksToWrite;
    //fileDescr->locate_offset = nbyte % BLOCK_SIZE_BYTES;
    block_store_fd_write(fs->BlockStore_fd,fd,fileDescr);
    free(fileDescr);
    free(tempBuffer);
    return bytes_written;
}


int fs_remove(FS_t *fs, const char *path)
{
    // Check for valid parameters
    if (fs == NULL || path == NULL || strlen(path) == 0) {
        return -1;
    }

    // Parse the path
    char* copy_path = (char*)calloc(1, 65535);
    if (copy_path == NULL) {
        return -1;
    }
    strcpy(copy_path, path);

    char** tokens;
    size_t count = 0;
    tokens = str_split(copy_path, '/', &count);
    free(copy_path);

    if (tokens == NULL) {
        return -1;
    }

    // Validate filenames
    for (size_t n = 0; n < count; n++) {
        if (!isValidFileName(*(tokens + n))) {
            // Free tokens before returning
            for (size_t i = 0; i < count; i++) {
                free(*(tokens + i));
            }
            free(tokens);
            return -1;
        }
    }

    // Find the parent directory and the file/directory to remove
    size_t parent_inode_ID = 0; // Start from root directory
    size_t indicator = 0;
    size_t target_parent_inode_ID = 0;
    int target_entry_index = -1;

    // Navigate to the parent directory
    for (size_t i = 0; i < count - 1; i++) {
        inode_t *parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
        if (parent_inode == NULL) {
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < count; j_idx++) {
                free(*(tokens + j_idx));
            }
            free(tokens);
            return -1;
        }

        block_store_inode_read(fs->BlockStore_inode, parent_inode_ID, parent_inode);

        if (parent_inode->fileType == 'd') {
            directoryFile_t *parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
            if (parent_data == NULL) {
                free(parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < count; j_idx++) {
                    free(*(tokens + j_idx));
                }
                free(tokens);
                return -1;
            }

            block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

            bool found = false;
            for (int j = 0; j < folder_number_entries; j++) {
                if (((parent_inode->vacantFile >> j) & 1) == 1 &&
                    strcmp((parent_data + j)->filename, *(tokens + i)) == 0) {
                    parent_inode_ID = (parent_data + j)->inodeNumber;
                    indicator++;
                    found = true;
                    break;
                }
            }

            free(parent_data);

            if (!found) {
                free(parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < count; j_idx++) {
                    free(*(tokens + j_idx));
                }
                free(tokens);
                return -1; // Path component not found
            }
        } else {
            free(parent_inode);
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < count; j_idx++) {
                free(*(tokens + j_idx));
            }
            free(tokens);
            return -1; // Not a directory
        }

        free(parent_inode);
    }

    // At this point, parent_inode_ID is the inode of the parent directory
    target_parent_inode_ID = parent_inode_ID;

    // Find the file/directory to remove in the parent directory
    inode_t *parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (parent_inode == NULL) {
        // Free tokens before returning
        for (size_t j_idx = 0; j_idx < count; j_idx++) {
            free(*(tokens + j_idx));
        }
        free(tokens);
        return -1;
    }

    block_store_inode_read(fs->BlockStore_inode, target_parent_inode_ID, parent_inode);

    if (parent_inode->fileType != 'd') {
        free(parent_inode);
        // Free tokens before returning
        for (size_t j_idx = 0; j_idx < count; j_idx++) {
            free(*(tokens + j_idx));
        }
        free(tokens);
        return -1; // Parent is not a directory
    }

    directoryFile_t *parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
    if (parent_data == NULL) {
        free(parent_inode);
        // Free tokens before returning
        for (size_t j_idx = 0; j_idx < count; j_idx++) {
            free(*(tokens + j_idx));
        }
        free(tokens);
        return -1;
    }

    block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

    size_t target_inode_ID = 0;
    for (int j = 0; j < folder_number_entries; j++) {
        if (((parent_inode->vacantFile >> j) & 1) == 1 &&
            strcmp((parent_data + j)->filename, *(tokens + count - 1)) == 0) {
            target_inode_ID = (parent_data + j)->inodeNumber;
            target_entry_index = j;
            break;
        }
    }

    if (target_entry_index == -1) {
        free(parent_data);
        free(parent_inode);
        // Free tokens before returning
        for (size_t j_idx = 0; j_idx < count; j_idx++) {
            free(*(tokens + j_idx));
        }
        free(tokens);
        return -1; // File/directory not found
    }

    // Get the inode of the file/directory to remove
    inode_t *target_inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (target_inode == NULL) {
        free(parent_data);
        free(parent_inode);
        // Free tokens before returning
        for (size_t j_idx = 0; j_idx < count; j_idx++) {
            free(*(tokens + j_idx));
        }
        free(tokens);
        return -1;
    }

    block_store_inode_read(fs->BlockStore_inode, target_inode_ID, target_inode);

    // Check if it's a directory and if it's empty
    if (target_inode->fileType == 'd') {
        // Check if directory is empty
        if (target_inode->vacantFile != 0) {
            free(target_inode);
            free(parent_data);
            free(parent_inode);
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < count; j_idx++) {
                free(*(tokens + j_idx));
            }
            free(tokens);
            return -1; // Directory not empty
        }

        // Free the directory's data block if it exists
        if (target_inode->directPointer[0] != 0) {
            block_store_release(fs->BlockStore_whole, target_inode->directPointer[0]);
        }
    } else {
        // It's a regular file, free all its data blocks

        // Free direct blocks
        for (int i = 0; i < 6; i++) {
            if (target_inode->directPointer[i] != 0) {
                block_store_release(fs->BlockStore_whole, target_inode->directPointer[i]);
            }
        }

        // Free indirect blocks
        if (target_inode->indirectPointer[0] != 0) {
            uint16_t *indirect_data = (uint16_t *)calloc(1, BLOCK_SIZE_BYTES);
            if (indirect_data != NULL) {
                block_store_read(fs->BlockStore_whole, target_inode->indirectPointer[0], indirect_data);

                // Free all blocks pointed to by the indirect block
                for (size_t i = 0; i < BLOCK_SIZE_BYTES / sizeof(uint16_t); i++) {
                    if (indirect_data[i] != 0) {
                        block_store_release(fs->BlockStore_whole, indirect_data[i]);
                    }
                }

                free(indirect_data);
            }

            // Free the indirect block itself
            block_store_release(fs->BlockStore_whole, target_inode->indirectPointer[0]);
        }

        // Close any open file descriptors for this file
        for (int fd = 0; fd < number_fd; fd++) {
            if (block_store_sub_test(fs->BlockStore_fd, fd)) {
                fileDescriptor_t *fd_data = (fileDescriptor_t *)calloc(1, sizeof(fileDescriptor_t));
                if (fd_data != NULL) {
                    block_store_fd_read(fs->BlockStore_fd, fd, fd_data);

                    if (fd_data->inodeNum == target_inode_ID) {
                        block_store_sub_release(fs->BlockStore_fd, fd);
                    }

                    free(fd_data);
                }
            }
        }
    }

    // Update parent directory
    parent_inode->vacantFile &= ~(1 << target_entry_index); // Clear the bit
    block_store_inode_write(fs->BlockStore_inode, target_parent_inode_ID, parent_inode);

    // Free the inode
    block_store_sub_release(fs->BlockStore_inode, target_inode_ID);

    // Clean up
    free(target_inode);
    free(parent_data);
    free(parent_inode);

    // Free tokens
    for (size_t j_idx = 0; j_idx < count; j_idx++) {
        free(*(tokens + j_idx));
    }
    free(tokens);

    return 0;
}

int fs_move(FS_t *fs, const char *src, const char *dst)
{
    // Check for valid parameters
    if (fs == NULL || src == NULL || dst == NULL || strlen(src) == 0 || strlen(dst) == 0) {
        return -1;
    }

    // Parse the source path
    char* copy_src = (char*)calloc(1, 65535);
    if (copy_src == NULL) {
        return -1;
    }
    strcpy(copy_src, src);

    char** src_tokens;
    size_t src_count = 0;
    src_tokens = str_split(copy_src, '/', &src_count);
    free(copy_src);

    if (src_tokens == NULL) {
        return -1;
    }

    // Parse the destination path
    char* copy_dst = (char*)calloc(1, 65535);
    if (copy_dst == NULL) {
        // Free src_tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        return -1;
    }
    strcpy(copy_dst, dst);

    char** dst_tokens;
    size_t dst_count = 0;
    dst_tokens = str_split(copy_dst, '/', &dst_count);
    free(copy_dst);

    if (dst_tokens == NULL) {
        // Free src_tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        return -1;
    }

    // Validate filenames
    for (size_t n = 0; n < src_count; n++) {
        if (!isValidFileName(*(src_tokens + n))) {
            // Free tokens before returning
            for (size_t i = 0; i < src_count; i++) {
                free(*(src_tokens + i));
            }
            free(src_tokens);
            for (size_t i = 0; i < dst_count; i++) {
                free(*(dst_tokens + i));
            }
            free(dst_tokens);
            return -1;
        }
    }

    for (size_t n = 0; n < dst_count; n++) {
        if (!isValidFileName(*(dst_tokens + n))) {
            // Free tokens before returning
            for (size_t i = 0; i < src_count; i++) {
                free(*(src_tokens + i));
            }
            free(src_tokens);
            for (size_t i = 0; i < dst_count; i++) {
                free(*(dst_tokens + i));
            }
            free(dst_tokens);
            return -1;
        }
    }

    // Find the source file/directory
    size_t src_parent_inode_ID = 0; // Start from root directory
    size_t src_indicator = 0;
    int src_entry_index = -1;

    // Navigate to the source parent directory
    for (size_t i = 0; i < src_count - 1; i++) {
        inode_t *parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
        if (parent_inode == NULL) {
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                free(*(src_tokens + j_idx));
            }
            free(src_tokens);
            for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                free(*(dst_tokens + k_idx));
            }
            free(dst_tokens);
            return -1;
        }

        block_store_inode_read(fs->BlockStore_inode, src_parent_inode_ID, parent_inode);

        if (parent_inode->fileType == 'd') {
            directoryFile_t *parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
            if (parent_data == NULL) {
                free(parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                    free(*(src_tokens + j_idx));
                }
                free(src_tokens);
                for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                    free(*(dst_tokens + k_idx));
                }
                free(dst_tokens);
                return -1;
            }

            block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

            bool found = false;
            for (int j = 0; j < folder_number_entries; j++) {
                if (((parent_inode->vacantFile >> j) & 1) == 1 &&
                    strcmp((parent_data + j)->filename, *(src_tokens + i)) == 0) {
                    src_parent_inode_ID = (parent_data + j)->inodeNumber;
                    src_indicator++;
                    found = true;
                    break;
                }
            }

            free(parent_data);

            if (!found) {
                free(parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                    free(*(src_tokens + j_idx));
                }
                free(src_tokens);
                for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                    free(*(dst_tokens + k_idx));
                }
                free(dst_tokens);
                return -1; // Path component not found
            }
        } else {
            free(parent_inode);
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                free(*(src_tokens + j_idx));
            }
            free(src_tokens);
            for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                free(*(dst_tokens + k_idx));
            }
            free(dst_tokens);
            return -1; // Not a directory
        }

        free(parent_inode);
    }

    // Find the source file/directory in the parent directory
    inode_t *src_parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (src_parent_inode == NULL) {
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1;
    }

    block_store_inode_read(fs->BlockStore_inode, src_parent_inode_ID, src_parent_inode);

    if (src_parent_inode->fileType != 'd') {
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1; // Parent is not a directory
    }

    directoryFile_t *src_parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
    if (src_parent_data == NULL) {
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1;
    }

    block_store_read(fs->BlockStore_whole, src_parent_inode->directPointer[0], src_parent_data);

    size_t src_inode_ID = 0;
    for (int j = 0; j < folder_number_entries; j++) {
        if (((src_parent_inode->vacantFile >> j) & 1) == 1 &&
            strcmp((src_parent_data + j)->filename, *(src_tokens + src_count - 1)) == 0) {
            src_inode_ID = (src_parent_data + j)->inodeNumber;
            src_entry_index = j;
            break;
        }
    }

    if (src_entry_index == -1) {
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1; // Source file/directory not found
    }

    // Get the source inode
    inode_t *src_inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (src_inode == NULL) {
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1;
    }

    block_store_inode_read(fs->BlockStore_inode, src_inode_ID, src_inode);

    // Find the destination parent directory
    size_t dst_parent_inode_ID = 0; // Start from root directory
    size_t dst_indicator = 0;

    // Navigate to the destination parent directory
    for (size_t i = 0; i < dst_count - 1; i++) {
        inode_t *parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
        if (parent_inode == NULL) {
            free(src_inode);
            free(src_parent_data);
            free(src_parent_inode);
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                free(*(src_tokens + j_idx));
            }
            free(src_tokens);
            for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                free(*(dst_tokens + k_idx));
            }
            free(dst_tokens);
            return -1;
        }
 
        block_store_inode_read(fs->BlockStore_inode, dst_parent_inode_ID, parent_inode);

        if (parent_inode->fileType == 'd') {
            directoryFile_t *parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
            if (parent_data == NULL) {
                free(parent_inode);
                free(src_inode);
                free(src_parent_data);
                free(src_parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                    free(*(src_tokens + j_idx));
                }
                free(src_tokens);
                for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                    free(*(dst_tokens + k_idx));
                }
                free(dst_tokens);
                return -1;
            }

            block_store_read(fs->BlockStore_whole, parent_inode->directPointer[0], parent_data);

            bool found = false;
            for (int j = 0; j < folder_number_entries; j++) {
                if (((parent_inode->vacantFile >> j) & 1) == 1 &&
                    strcmp((parent_data + j)->filename, *(dst_tokens + i)) == 0) {
                    dst_parent_inode_ID = (parent_data + j)->inodeNumber;
                    dst_indicator++;
                    found = true;
                    break;
                }
            }

            free(parent_data);

            if (!found) {
                free(parent_inode);
                free(src_inode);
                free(src_parent_data);
                free(src_parent_inode);
                // Free tokens before returning
                for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                    free(*(src_tokens + j_idx));
                }
                free(src_tokens);
                for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                    free(*(dst_tokens + k_idx));
                }
                free(dst_tokens);
                return -1; // Path component not found
            }
        } else {
            free(parent_inode);
            free(src_inode);
            free(src_parent_data);
            free(src_parent_inode);
            // Free tokens before returning
            for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
                free(*(src_tokens + j_idx));
            }
            free(src_tokens);
            for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
                free(*(dst_tokens + k_idx));
            }
            free(dst_tokens);
            return -1; // Not a directory
        }

        free(parent_inode);
    }

    // Check if the destination parent directory exists and is a directory
    inode_t *dst_parent_inode = (inode_t *)calloc(1, sizeof(inode_t));
    if (dst_parent_inode == NULL) {
        free(src_inode);
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1;
    }
    if (src_inode->fileType == 'd' && src_inode_ID == dst_parent_inode_ID) {
        free(src_inode);
        free(dst_parent_inode);
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens and return error
        return -1;
    }

    block_store_inode_read(fs->BlockStore_inode, dst_parent_inode_ID, dst_parent_inode);

    if (dst_parent_inode->fileType != 'd') {
        free(dst_parent_inode);
        free(src_inode);
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1; // Destination parent is not a directory
    }

    // Check if destination directory has space for a new entry
    directoryFile_t *dst_parent_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
    if (dst_parent_data == NULL) {
        free(dst_parent_inode);
        free(src_inode);
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1;
    }

    // Allocate data block for destination parent if needed
    if (dst_parent_inode->directPointer[0] == 0) {
        size_t dst_parent_data_ID = block_store_allocate(fs->BlockStore_whole);
        if (dst_parent_data_ID == SIZE_MAX) {
            free(dst_parent_data);
            free(dst_parent_inode);
            free(src_inode);
            free(src_parent_data);
            free(src_parent_inode);
            // Free tokens before returning
            for (size_t i = 0; i < src_count; i++) {
                free(*(src_tokens + i));
            }
            free(src_tokens);
            for (size_t i = 0; i < dst_count; i++) {
                free(*(dst_tokens + i));
            }
            free(dst_tokens);
            return -1; // No space for data block
        }
        dst_parent_inode->directPointer[0] = dst_parent_data_ID;
        block_store_inode_write(fs->BlockStore_inode, dst_parent_inode_ID, dst_parent_inode);
    }

    block_store_read(fs->BlockStore_whole, dst_parent_inode->directPointer[0], dst_parent_data);

    // Check if a file/directory with the same name already exists in the destination
    for (int j = 0; j < folder_number_entries; j++) {
        if (((dst_parent_inode->vacantFile >> j) & 1) == 1 &&
            strcmp((dst_parent_data + j)->filename, *(dst_tokens + dst_count - 1)) == 0) {
            free(dst_parent_data);
            free(dst_parent_inode);
            free(src_inode);
            free(src_parent_data);
            free(src_parent_inode);
            // Free tokens before returning
            for (size_t i = 0; i < src_count; i++) {
                free(*(src_tokens + i));
            }
            free(src_tokens);
            for (size_t i = 0; i < dst_count; i++) {
                free(*(dst_tokens + i));
            }
            free(dst_tokens);
            return -1; // File/directory with same name already exists
        }
    }

    // Find an empty slot in the destination directory
    int dst_entry_index = -1;
    for (int j = 0; j < folder_number_entries; j++) {
        if (((dst_parent_inode->vacantFile >> j) & 1) == 0) {
            dst_entry_index = j;
            break;
        }
    }

    if (dst_entry_index == -1) {
        free(dst_parent_data);
        free(dst_parent_inode);
        free(src_inode);
        free(src_parent_data);
        free(src_parent_inode);
        // Free tokens before returning
        for (size_t i = 0; i < src_count; i++) {
            free(*(src_tokens + i));
        }
        free(src_tokens);
        for (size_t i = 0; i < dst_count; i++) {
            free(*(dst_tokens + i));
        }
        free(dst_tokens);
        return -1; // Destination directory is full
    }

    // Add the file/directory to the destination directory
    dst_parent_inode->vacantFile |= (1 << dst_entry_index);
    strcpy((dst_parent_data + dst_entry_index)->filename, *(dst_tokens + dst_count - 1));
    (dst_parent_data + dst_entry_index)->inodeNumber = src_inode_ID;

    // Remove the file/directory from the source directory
    src_parent_inode->vacantFile &= ~(1 << src_entry_index);

    // Write the changes back
    block_store_write(fs->BlockStore_whole, dst_parent_inode->directPointer[0], dst_parent_data);
    block_store_inode_write(fs->BlockStore_inode, dst_parent_inode_ID, dst_parent_inode);

    block_store_write(fs->BlockStore_whole, src_parent_inode->directPointer[0], src_parent_data);
    block_store_inode_write(fs->BlockStore_inode, src_parent_inode_ID, src_parent_inode);

    // Clean up
    free(dst_parent_data);
    free(dst_parent_inode);
    free(src_inode);
    free(src_parent_data);
    free(src_parent_inode);

    // Free tokens
    for (size_t j_idx = 0; j_idx < src_count; j_idx++) {
        free(*(src_tokens + j_idx));
    }
    free(src_tokens);
    for (size_t k_idx = 0; k_idx < dst_count; k_idx++) {
        free(*(dst_tokens + k_idx));
    }
    free(dst_tokens);

    return 0;
}
int fs_link(FS_t *fs, const char *src, const char *dst) {
    // Step 1: Parameter validation
    if (fs == NULL || src == NULL || dst == NULL || strlen(src) == 0 || strlen(dst) == 0) {
    return -1;
    }
    
    // Check if destination is root (cannot create a hardlink to root)
    if (strcmp(dst, "/") == 0) {
    return -1;
    }
    
    // Step 2: Parse paths and find inodes
    char* src_copy = (char*)calloc(1, 65535);
    char* dst_copy = (char*)calloc(1, 65535);
    if (!src_copy || !dst_copy) {
    free(src_copy);
    free(dst_copy);
    return -1;
    }
    strcpy(src_copy, src);
    strcpy(dst_copy, dst);
    char** src_tokens;
    char** dst_tokens;
    size_t src_count = 0, dst_count = 0;
    // Split paths into components
    src_tokens = str_split(src_copy, '/', &src_count);
    dst_tokens = str_split(dst_copy, '/', &dst_count);
    free(src_copy);
    free(dst_copy);
    if (!src_tokens || !dst_tokens) {
    if (src_tokens) {
    for (size_t i = 0; i < src_count; i++) {
    free(*(src_tokens + i));
    }
    free(src_tokens);
    }
    if (dst_tokens) {
    for (size_t i = 0; i < dst_count; i++) {
    free(*(dst_tokens + i));
    }
    free(dst_tokens);
    }
    return -1;
    }
    // Validate path components
    for (size_t i = 0; i < src_count; i++) {
    if (!isValidFileName(*(src_tokens + i))) {
    // Free memory and return error
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    }
    for (size_t i = 0; i < dst_count; i++) {
    if (!isValidFileName(*(dst_tokens + i) )) {
    // Free memory and return error
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    }
    // Step 3: Locate Source File/Directory
    size_t src_parent_inode_id = 0; // Start from the root
    size_t src_inode_id = 0;
    bool src_found = false;
    inode_t current_inode;
    directoryFile_t *dir_data = (directoryFile_t *)calloc(1, BLOCK_SIZE_BYTES);
    if (!dir_data) {
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Traverse path to find the source inode
    for (size_t i = 0; i < src_count; i++) {
    block_store_inode_read(fs->BlockStore_inode, src_parent_inode_id, &current_inode);
    if (current_inode.fileType != 'd') {
    // Not a dir, can't navigate further
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    block_store_read(fs->BlockStore_whole, current_inode.directPointer[0], dir_data);
    bool found_component = false;
    for (int j = 0; j < folder_number_entries; j++) {
    if (((current_inode.vacantFile >> j) & 1) &&
    strcmp((dir_data + j)->filename, *(src_tokens + i)) == 0) {
    src_parent_inode_id = (dir_data + j)->inodeNumber;
    found_component = true;
    break;
    }
    }
    if (!found_component) {
    // Path component not found
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    }
    // Source file found!!
    src_inode_id = src_parent_inode_id;
    src_found = true;
    // If source not found, return error :(
    if (!src_found) {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Read source inode
    inode_t src_inode;
    block_store_inode_read(fs->BlockStore_inode, src_inode_id, &src_inode);
    // Step 4: Locate Destination Parent Directory
    size_t dst_parent_inode_id = 0; // Start from root
    bool dst_parent_found = false;
    // Traverse path to find parent dir of destination
    for (size_t i = 0; i < dst_count - 1; i++) {
    block_store_inode_read(fs->BlockStore_inode, dst_parent_inode_id, &current_inode);
    if (current_inode.fileType != 'd') {
    // Not a directory, can't navigate more :(
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    block_store_read(fs->BlockStore_whole, current_inode.directPointer[0], dir_data);
    bool found_component = false;
    for (int j = 0; j < folder_number_entries; j++) {
    if (((current_inode.vacantFile >> j) & 1) &&
    strcmp((dir_data + j)->filename, *(dst_tokens + i)) == 0) {
    dst_parent_inode_id = (dir_data + j)->inodeNumber;
    found_component = true;
    break;
    }
    }
    if (!found_component) {
    // Path component not found :/
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    }
    dst_parent_found = true;
    // If destination parent not found, return error
    if (!dst_parent_found) {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Read destination parent inode
    inode_t dst_parent_inode;
    block_store_inode_read(fs->BlockStore_inode, dst_parent_inode_id, &dst_parent_inode);
    // Check if parent is a dir
    if (dst_parent_inode.fileType != 'd') {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Step 5: Check Destination Doesn't Exist and Parent Directory Has Space
    block_store_read(fs->BlockStore_whole, dst_parent_inode.directPointer[0], dir_data);
    // Ensure destination doesn't already exist
    for (int i = 0; i < folder_number_entries; i++) {
    if (((dst_parent_inode.vacantFile >> i) & 1) &&
    strcmp((dir_data + i)->filename, *(dst_tokens + dst_count - 1)) == 0) {
    // Destination already exists :0
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    }
    // Find free slot in parent directory
    int free_slot = -1;
    for (int i = 0; i < folder_number_entries; i++) {
    if (!((dst_parent_inode.vacantFile >> i) & 1)) {
    free_slot = i;
    break;
    }
    }
    // If no free slot, dir is full
    if (free_slot == -1) {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Alloc data block for parent dir if needed
    if (dst_parent_inode.directPointer[0] == 0) {
    size_t block_id = block_store_allocate(fs->BlockStore_whole);
    if (block_id >= BLOCK_STORE_AVAIL_BLOCKS) {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    dst_parent_inode.directPointer[0] = block_id;
    // init directory data
    memset(dir_data, 0, BLOCK_SIZE_BYTES);
    }
    // Step 6: Check Link Count and Create Link
    if (src_inode.linkCount >= 255) {
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return -1;
    }
    // Increment link count in source inode
    src_inode.linkCount++;
    block_store_inode_write(fs->BlockStore_inode, src_inode_id, &src_inode);
    // Create directory entry for the new link
    strcpy((dir_data + free_slot)->filename, *(dst_tokens + dst_count - 1));
    (dir_data + free_slot)->inodeNumber = src_inode_id;
    // Update parent directory bitmap and write back to block store
    dst_parent_inode.vacantFile |= (1 << free_slot);
    block_store_inode_write(fs->BlockStore_inode, dst_parent_inode_id, &dst_parent_inode);
    block_store_write(fs->BlockStore_whole, dst_parent_inode.directPointer[0], dir_data);
    // Free the memory, oy vey, this function was totally so fun ;(
    free(dir_data);
    for (size_t j = 0; j < src_count; j++) {
    free(*(src_tokens + j));
    }
    free(src_tokens);
    for (size_t j = 0; j < dst_count; j++) {
    free(*(dst_tokens + j));
    }
    free(dst_tokens);
    return 0;
    }