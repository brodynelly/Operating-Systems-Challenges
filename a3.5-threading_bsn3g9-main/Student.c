#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "ImageFile.h"
#include "Student.h"

// thread to lock everythign 
pthread_mutex_t lock;

typedef struct {
    PFileHandle fileHandle;
    unsigned long offset;
    unsigned long size;
} ThreadData;

void InitTesting(void) {
    // Empty initialization function
}

int StudentWriteDelegate(FILE* file, uint8_t* data, unsigned long start, unsigned long size) {
    pthread_mutex_lock(&lock);

    if (!file || !data || size == 0) {
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if (fseek(file, start, SEEK_SET)) {
        pthread_mutex_unlock(&lock);
        return 0;
    }
    if (fwrite(data + start, 1, size, file) != size) {
        pthread_mutex_unlock(&lock);
        return 0;
    }
    pthread_mutex_unlock(&lock);
    return 1;
}

void* WriteThread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int result = WriteToImageFile(data->fileHandle, StudentWriteDelegate, data->offset, data->size);

    if (!result) {
        fprintf(stderr, "Error writing chunk at offset %lu\n", data->offset);
    } else {
        printf("p_T %lu success, ", pthread_self());
    }

    free(data);
    return NULL;
}

void WriteImage(PFileHandle fileHandle) {
    // init mutex for completion
    pthread_mutex_init(&lock, NULL);

    unsigned long totalSize = GetDataSize(fileHandle);
    unsigned long numChunks = (totalSize + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
    pthread_t threads[numChunks];

    printf("Starting write process with %lu chunks\n", numChunks);

    for (unsigned long i = 0; i < numChunks; i++) {
        ThreadData* data = malloc(sizeof(ThreadData));
        if (!data) {
            fprintf(stderr, "Failed to allocate memory for chunk %lu\n", i);
            continue;
        }

        data->fileHandle = fileHandle;
        data->offset = i * MAX_BLOCK_SIZE;
        data->size = (i == numChunks - 1) ? totalSize - data->offset : MAX_BLOCK_SIZE;

        if (pthread_create(&threads[i], NULL, WriteThread, data)) {
            fprintf(stderr, "Failed to create thread for chunk %lu\n", i);
            free(data);
        }
    }

    for (unsigned long i = 0; i < numChunks; i++) {
        pthread_join(threads[i], NULL);
        printf(" %lu completed", i);
    }

    pthread_mutex_destroy(&lock);
}