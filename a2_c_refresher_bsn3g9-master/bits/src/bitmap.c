#include "../include/bitmap.h"

// data is an array of uint8_t and needs to be allocated in bitmap_create
//      and used in the remaining bitmap functions. You will use data for any bit operations and bit logic
// bit_count the number of requested bits, set in bitmap_create from n_bits
// byte_count the total number of bytes the data contains, set in bitmap_create


// useful link:
// https://web.archive.org/web/20220103112233/http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

// typedef struct bitmap 
// {
//     uint8_t *data;
//     size_t bit_count, byte_count;
// } bitmap_t;

/// Creates a bitmap to contain 'n' bits (zero initialized)
/// \param n_bits
/// \return New bitmap pointer, NULL on error
///
// 
bitmap_t *bitmap_create(size_t n_bits)
{
    // gaurd statment against invalid params 
    if ( n_bits == 0 ) { return NULL; }
    
    // allocate mem 
    bitmap_t *bitmap = (bitmap_t *)malloc(sizeof(bitmap_t));
    if (!bitmap) { return NULL; }

    // Scalc the number of bytes needed
    bitmap->bit_count = n_bits;
    bitmap->byte_count = (n_bits + 7) / 8;

    // allocate memory for data and set bytes to zero
    bitmap->data = (uint8_t *)calloc(bitmap->byte_count, sizeof(uint8_t));
    if (!bitmap->data) { 
        free(bitmap); 
        return NULL;
    }

    return bitmap; 
}

///
/// Sets the requested bit in bitmap
/// \param bitmap The bitmap
/// \param bit The bit to set
///
bool bitmap_set(bitmap_t *const bitmap, const size_t bit)
{
    // gaurd statement for invalid params
    if (bitmap == NULL || bit >= bitmap->bit_count) {
        return false;  
    }

    size_t byte_index = bit / 8;
    size_t bit_index = bit % 8;

    //  check if the data in the bitmap is correctly allocated
    if (bitmap->data == NULL) {
        return false;  
    }

    // set the bit in data
    bitmap->data[byte_index] |= (1 << bit_index);
    return true;
}

///
/// Clears requested bit in bitmap
/// \param bitmap The bitmap
/// \param bit The bit to be cleared
///
bool bitmap_reset(bitmap_t *const bitmap, const size_t bit)
{
    // gaurd statment against invalid params 
    if ( bitmap == NULL ||bit >= bitmap->bit_count ) { return false; }

    // Calculate the byte index and the bit index within that byte
    size_t byte_index = bit / 8;
    size_t bit_index = bit % 8;

    // Clear the bit in the corresponding byte
    bitmap->data[byte_index] &= ~(1 << bit_index);
    return true;
}

///
/// Returns bit in bitmap
/// \param bitmap The bitmap
/// \param bit The bit to queried
/// \return State of requested bit
//
bool bitmap_test(const bitmap_t *const bitmap, const size_t bit)
{
    // gaurd statement against invalid params
    if ( bitmap == NULL || bit == 0 || bit >= bitmap->bit_count) { return false; } 
 
    // test bit statements
    if ((bitmap->data[bit / 8] & (1 << (bit % 8))) != 0) {
        return true; 
    } 
    // else return false
    return false; 
}

/// Find the first set bit
/// \param bitmap The bitmap
/// \return The first one bit address, SIZE_MAX on error/Not found
///
size_t bitmap_ffs(const bitmap_t *const bitmap)
{
    // gaurd statement against invalid params 
    if ( bitmap == NULL ) { return SIZE_MAX; } 
   
    // iterate through the bitmap
    for (size_t i = 0; i < bitmap->bit_count; i++) {
        if (bitmap_test(bitmap, i)) {
            return i;
        }
    }
    return SIZE_MAX;
}

/// Find first zero bit
/// \param bitmap The bitmap
/// \return The first zero bit address, SIZE_MAX on error/Not found
///
size_t bitmap_ffz(const bitmap_t *const bitmap)
{
    // Guard against invalid params
    if (!bitmap || !bitmap->data || bitmap->byte_count == 0 ) {
        return SIZE_MAX;
    }

    size_t i = 0;
    uint8_t *current  = bitmap->data; 
    // Iterate through each byte
    for (; i < bitmap->byte_count; ++i, ++current) {
        // all byte are set
        if (*current != 0xFF) {
            // fint first zero bit
            for (size_t bit = 0; bit < 8; ++bit) {
                if ((*current & (1 << bit)) == 0) {
                    return (i * 8) + bit;
                }
            }
        }
    }
    return SIZE_MAX;
}

/// Destructs and destroys bitmap object
/// \param bit The bitmap
/// \return The Success or Failure of destruct and destroy bitmap object
///
bool bitmap_destroy(bitmap_t *bitmap)
{
    // gaurd statemtn 
    if ( bitmap == NULL ) return false;

    // free allocated memory and bitmap
    free(bitmap->data);
    free(bitmap);
    return true;
}
