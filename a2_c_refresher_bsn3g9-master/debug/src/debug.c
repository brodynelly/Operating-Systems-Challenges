#include "../include/debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
// Protected function, that only this .c can use
int comparator_func(const void *a, const void *b) {
    return (*(uint16_t *)a - *(uint16_t *)b);  // Compare values, not pointers
}

bool terrible_sort(uint16_t *data_array, const size_t value_count) {
    // Allocate memory for the sorting array
    uint16_t *sorting_array = malloc(value_count * sizeof(*data_array));
    if (sorting_array == NULL) {
        return false;  // Return false if memory allocation failed
    }

    // Copy the data into sorting_array
    for (int i = 0; i < value_count; ++i) {
        sorting_array[i] = data_array[i];
    }

    // Sort the array
    qsort(sorting_array, value_count, sizeof(uint16_t), comparator_func);  // Correct qsort call

    // Check if the array is sorted
    bool sorted = true;
    for (int i = 0; i < value_count - 1; ++i) {  // Compare adjacent elements
        if (sorting_array[i] > sorting_array[i + 1]) {
            sorted = false;  // Mark as unsorted if any element is greater than the next one
            break;  // Early exit if we found an unsorted pair
        }
    }

    // If sorted, copy the sorted array back to the original data_array
    if (sorted) {
        memcpy(data_array, sorting_array, value_count * sizeof(*data_array));  // Copy sorted array back
    }

    // Free the allocated memory
    free(sorting_array);

    return sorted;
}