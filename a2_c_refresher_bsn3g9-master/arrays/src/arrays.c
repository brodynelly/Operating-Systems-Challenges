#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/arrays.h"

// LOOK INTO MEMCPY, MEMCMP, FREAD, and FWRITE
// HINT: MEMCPY and MEMCMP are useful functions to use

// Copy the contents from source array into destination array
// \param src the array the will be copied into the destination array
// \param dst the array that will be copied from the source array
// \param elem_size the number of bytes each array element uses
// \param elem_count the number of elements in the source array
// return true if operation was successful, else false
bool array_copy(const void *src, void *dst, const size_t elem_size, const size_t elem_count)
{
	// gaurd statement for the function 
	if( src == NULL || dst == NULL || elem_size==0 ||elem_count == 0 ) { return false; } 
	// create a copy of arrays for compiler read arrays
	for( size_t i = 0; i < elem_count; ++i ) {
		void *src_copy = (char *)src + i*elem_size; 
		void *dst_copy = (char *)dst + i*elem_size; 
	
	// use memcopy to copy source array to destination array
		memcpy(dst_copy, src_copy, elem_size);
	}
	return true; 
}

bool array_is_equal(const void *data_one, void *data_two, const size_t elem_size, const size_t elem_count)
{
	// gaurd statement from invalid params 
// Compares if two arrays contain the same contents
    if ( data_one == NULL || data_two == NULL || elem_size == 0 || elem_count == 0) { return false; }
	// compaire each element in arrays
	for ( size_t i = 0; i < elem_count; ++i){
		void *data_one_cpy = (void *)data_one + i *elem_size; 
		void *data_two_cpy = (void *)data_two + i * elem_size; 

		if ( memcmp(data_one_cpy, data_two_cpy, elem_size) == 0 ){
		       continue; 
		}
 		else {
			return false; 
		}		
	}
	return true; 
}
// Attempts to locate the target from an array
// \param data the data that may contain the target
// \param target the target that may be in the data
// \param elem_size the number of bytes each array element uses and same as the target
// \param elem_count the number of elements in the data array
// returns an index to the located target, else return -1 for failure

ssize_t array_locate(const void *data, const void *target, const size_t elem_size, const size_t elem_count)
{
    if ( data == NULL || target == NULL || elem_size == 0 || elem_count == 0) { return -1; }
	// iterate through the loop 
    for (size_t i = 0; i < elem_count; ++i) {
		void *data_cpy = (void *)data + i *elem_size;

		if( memcmp( data_cpy, target, elem_size) == 0){
			return i; 
		}
	}
	return -1; 
}

// Writes an array into a binary file
// \param src_data the array the will be wrote into the destination file
// \param dst_file the file that will contain the wrote src_data
// \param elem_size the number of bytes each array element uses
// \param elem_count the number of elements in the source array
// return true if operation was successful, else false
bool array_serialize(const void *src_data, const char *dst_file, const size_t elem_size, const size_t elem_count)
{
	if ( src_data == NULL || dst_file == NULL || elem_size == 0 || elem_count == 0 ) { return false; }

	// detect a /n 
	size_t len = strlen(dst_file);
	for (size_t i = len; i > 0; --i) {
		if( (dst_file[len - 1] == '\n' || dst_file[len - 1] == ' ')){
			return false; 
		}
	}

	// need to declare the pointer of file from dst_file 
	FILE *fp; 
	fp = fopen( dst_file, "wb" ); 

	if ( fp == NULL ) { return false; }

	//.. write in the file 
	size_t written = fwrite(src_data, elem_size, elem_count, fp );
   	printf("Number of elements written: %zu\n", written);

	// terinary to return if file was written or not 
	fclose(fp); 
	return (written == elem_count ); 
}

// Reads an array from a binary file
// \param src_file the source file that contains the array to be read into the destination array
// \param dst_data the array that will contain the data stored inthe source file
// \param elem_size the number of bytes each array element uses of the destination array
// \param elem_count the number of elements in the destination array
// return true if operation was successful, else false
bool array_deserialize(const char *src_file, void *dst_data, const size_t elem_size, const size_t elem_count)
{
	// gaurd statement for invalid params
	if ( dst_data == NULL || src_file == NULL || elem_size == 0 || elem_count == 0 ) { return false; }

	// detect a \n 
	size_t len = strlen(src_file);
	if (len > 0 && (src_file[len - 1] == '\n' || src_file[len - 1] == ' ')) {
    	return false;  // Invalid if the filename ends with space or newline
	}

	// need to declare the pointer of file from dst_file 
	FILE *fp; 
	fp = fopen( src_file, "rb" ); 

	if ( fp == NULL ) { return false; }

	//write in the array 
	size_t written = 0;
	for (size_t i = 0; i < elem_count; ++i) {
		char *dst_data_cpy = (char *)dst_data + i * elem_size;

		size_t result = fread(dst_data_cpy, elem_size, 1, fp);
		if (result != 1) { fclose(fp); return false; }

		printf("Number of elements written: %zu\n", result);

		written += result; 
	}
    printf("Number of elements written: %zu\n", written);

	fclose(fp); 
	return (written == elem_count);

}

