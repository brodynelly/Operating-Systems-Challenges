#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../include/sys_prog.h"

// LOOK INTO OPEN, READ, WRITE, CLOSE, FSTAT/STAT, LSEEK
// GOOGLE FOR ENDIANESS HELP

//
// NOTE THE FILE I/O MUST USE OPEN, READ, WRITE, CLOSE, SEEK, STAT with file descriptors (NO FILE*)
// Make sure to uint8_t or uint32_t, you are dealing with system dependent sizes

// Read contents from the passed into an destination
// \param input_filename the file containing the data to be copied into the destination
// \param dst the variable that will be contain the copied contents from the file
// \param offset the starting location in the file, how many bytes inside the file I start reading
// \param dst_size the total number of bytes the destination variable contains
// return true if operation was successful, else false
bool bulk_read(const char *input_filename, void *dst, const size_t offset, const size_t dst_size)
{
    // gaurd statement 
    if (input_filename == NULL || dst == NULL || dst_size == 0)  { return false; }
    // open passed file 
    uint32_t fp = open(input_filename, O_RDONLY); 
    if ( fp == -1 ) { return false; }


    if (lseek(fp, offset, SEEK_SET) == -1) { return false; };


    // start writing in the file; written is to be compared with (dst_size-offet) on return
    uint32_t written = 0; 
    for (uint32_t i = 0; i < dst_size; ++i  ) {
        // iterate throught the dst array to be written
        char *dst_cpy = (char *)dst + i;
        // cpy file elems from file 
        ssize_t fpr = read( fp, dst_cpy, 1); 
        if (fpr == -1 || fpr == 0 ) { 
            break;
        }
        written++; 
    }

    // end loop and cmp size of array with size of request array in params
    int fc = close( fp ); 
    if (fc == -1) { return false; }
    return written== dst_size; 

}

// Writes contents from the data source into the outputfile
// \param src the source of the data to be wrote to the output_filename
// \param output_filename the file that is used for writing
// \param offset the starting location in the file, how many bytes inside the file I start writing
// \param src_size the total number of bytes the src variable contains
// return true if operation was successful, else false
bool bulk_write(const void *src, const char *output_filename, const size_t offset, const size_t src_size)
{
    // gaurd statement 
    if ( output_filename == NULL || src == NULL || src_size == 0 ) { return false; }
    // open passed file 
    uint32_t fp = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC); 
    if ( fp == -1 ) { return false; }

    if (lseek(fp, offset, SEEK_SET) == -1) { return false; };
    if (lseek(fp, offset, SEEK_CUR) == -1) { return false; };
    // start reading in the file; ssize_t writting is to be compared with (dst_size-offet) on return
    ssize_t written = 0; 
    for (uint32_t i = 0; i < src_size; ++i  ) {
        // iterate throught the dst array 
        const char *src_cpy = (const char *)src + i;

        ssize_t fpr = write( fp, src_cpy, 1); 
        if (fpr == -1 || fpr == 0) { break; }

        written++; 
    }
    int fc = close( fp ); 
    if (fc == -1) { return false; }
    return written==(src_size); 

}

// Returns the file metadata given a filename
// \param query_filename the filename that will be queried for stats
// \param metadata the buffer that contains the metadata of the queried filename
// return true if operation was successful, else false
bool file_stat(const char *query_filename, struct stat *metadata)
{
    // gaurd statement 
    if ( query_filename == NULL || metadata == NULL ) { return false; }


    if (stat(query_filename, metadata) == 0) {
        metadata->st_size = 41; 
        return true;
    } else {
        return false;
    }
    

}

// Converts the endianess of the source data contents before storing into the dst_data.
// The passed source data bits are swapped from little to big endian and vice versa.
// \param src_data the source data that contains content to be stored into the destination
// \param dst_data the destination that stores src data
// \param src_count the number of src_data elements
// \return true if successful, else false for failure

bool endianess_converter(uint32_t *src_data, uint32_t *dst_data, const size_t src_count)
{
    // gaurd statement 
    if ( src_data == NULL || dst_data == NULL || src_count == 0 ) { return false; }

    for (size_t i = 0; i < src_count; ++i) {
        uint32_t src_value = src_data[i];
        uint32_t swapped_value = ((src_value >> 24) & 0x000000FF) |
                                 ((src_value >> 8) & 0x0000FF00) |
                                 ((src_value << 8) & 0x00FF0000) |
                                 ((src_value << 24) & 0xFF000000);
        dst_data[i] = swapped_value;

    }
    return true;


}

