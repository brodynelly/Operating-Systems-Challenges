#include "../include/allocation.h"
#include <stdlib.h>
#include <stdio.h>

/// Allocates an array of n elements of size member_size in bytes utilizing calloc if the clear flag is set, malloc otherwise.
///	Testing with certain values will show different behavior between calloc and malloc.
/// \param size_t - member size in bytes (can be gotten using the sizeof operator e.g. sizeof(int)
/// \param size_t - number of members in the array
///	\param bool -   1 will clear memory (calling calloc), 0 will not (calling malloc).
/// \return void* - pointer to allocated memory.
void* allocate_array(size_t member_size, size_t nmember,bool clear)
{
    if ( member_size == 0 || nmember == 0 ) {
        return NULL; 
    }
    // memeory to be declared using calloc 
    if (clear) {
        // allocate memeory of size memeber_size for nmemeber units
        char *array = (char*)calloc(nmember, member_size ); 
        if (!array) return NULL;
        return (void *)array; 

    }
    // memeory to be declared using malloc
    else if ( !clear ) {
        // allocate memeory of size memeber_size for nmemeber units
        char *array = (char*)malloc(nmember* member_size ); 
        if (!array) return NULL;
        return (void *)array; 
    }
    else { // invalid clear {
        return NULL; 
    }

}

/// Realloc
/// - Does not initialize expanded memory
/// - Original contents remain unchanged
/// - If passed a null pointer then a malloc will occur.
/// - May move memory to a new location

/// Simple wrapper around realloc.
/// \param void* - pointer to memory to resize.
/// \param size_t - size of memory to allocate
/// \return void* - pointer to reallocated memory region, may be same as original pointer.
void* reallocate_array(void* ptr, size_t size)
{
    // gaurd statement 
    if ( ptr == NULL || size <= 0 ) return NULL; 
    return realloc ( ptr, size ); 

}

// Free
// - Should not be called on a null pointer or a pointer not received from allocation.
// - Free does not reinitialize the memory region.
// - An "Invalid Pointer" error may be a sign of bad memory operations or an overflow from a memset, memcpy, or allocation or freeing a pointer twice.
// - If the received pointer is null no operation is performed.

/// Wrapper around free. Frees memory and sets received pointer to NULL.
/// \param void* - pointer to memory to free.
/// \return Nothing
void deallocate_array(void** ptr)
{
    //check if the pointer is null 
    if ( *ptr == NULL ) return NULL; 
    // deallocate the pointer
    free(*ptr); 
    *ptr = NULL; 

    // no return 
}



// Heap & Stack
// - Local variables are allocated on the stack
// - Large local variable can overflow the stack as stack space is limited (the stack is shared with the functions your program calls as well)
// - When a stack variable leaves scope it is popped from the stack meaning you cannot return a local variable (stack variable) from a function.
// - Heap variables are allocated in memory (or in other places, e.g. using MMAP)
// - Heap variables can be vastly larger than stack variables
// - A heap variable remains available until it is freed, it is the programmers job to do so.
// - A heap variable that is not freed results in a memory leak, such leaks can be found using valgrind.
// - An overflow of the heap can cause serious issues in other parts of the program that may not be easily found. Such issues can usually be found with valgrind -v
// - Allocation of stack variables is faster as you only need to alter the stack pointer.

/// Takes a file name and reads a line into a newly allocated buffer
/// \param char* - filename to read from
/// \return char* - Pointer to malloced heap space containing buffer
char* read_line_to_buffer(char* filename)
{
    if (filename == NULL) {
        return NULL; 
    }

    // opens the file 
    FILE * file = fopen(filename, "r"); 
    // allocates char array into allocated buffer 
    char * buffer = (char*)malloc(sizeof(char)*BUFSIZ); 
    fgets(buffer, BUFSIZ, file); 
    fclose(file); 

    return buffer; 
}
