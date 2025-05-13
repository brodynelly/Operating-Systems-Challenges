#ifndef _ImageFile_H
#define _ImageFile_H

#include <stdio.h>
#include <stdint.h>


///////////////////////////////////////////////////////////////////////////////
// Symbolic Constants
///////////////////////////////////////////////////////////////////////////////
#define MAX_BLOCK_SIZE  (16*1024)
#define MAX_THREADS 1024


///////////////////////////////////////////////////////////////////////////////
// Data Types
///////////////////////////////////////////////////////////////////////////////
typedef enum
{
    PIC_C,
    PIC_M,
    PIC_O,
    PIC_P,
} IMAGE;

typedef int (*WRITEFUNCTION)(FILE * fp, uint8_t * pData, unsigned long startLocation, unsigned long size);

// Forward Definition
typedef struct _FileHandle * PFileHandle;

///////////////////////////////////////////////////////////////////////////////
// Prototypes
///////////////////////////////////////////////////////////////////////////////

// OpenImageFileForRead()
//      image - IMAGE variable indicating about which image to open a file for
//
// returns a PFileHandle defining the image file
PFileHandle  OpenImageFileForRead(IMAGE image);

// OpenImageFileForWrite()
//      image - IMAGE variable indicating about which image to open a file for
//
// returns a PFileHandle defining the image file
PFileHandle OpenImageFileForWrite(IMAGE image);

// WriteToImageFile
//      pFH - PFileHandle for the file to be written to
//      delegate - Function to be called that is allowed to use fwrite()
//      startLocation - location where to write this in the file
//      size - how much data to write in bytes
// returns 0 for failure or 1 for success
int WriteToImageFile(PFileHandle pFH, WRITEFUNCTION delegate, unsigned long startLocation, unsigned long size);

// CloseImageFile()
//      pFH - PFileHandle to be closed
//
// returns 0 for failure or 1 for success
int CloseImageFile(PFileHandle pFH);

// GetDataSize()
//      pFH - PFileHandle to an open image file
//
// returns 0 on error or the size of the entire data of the image
unsigned long GetDataSize(PFileHandle pFH);

// IsFileCorrect()
//      pFH - PFileHandle to an open image file
//
// returns 0 if the file is different than the original data or 1 if it is the same
int IsFileCorrect(PFileHandle pFH);

// TestFile()
//      image - identifier indicating which image to use
//
// Runs appropriate tests for given image
void TestFile(IMAGE image);

#endif