#include "ImageFile.h"

// Student-written functions
void WriteImage(PFileHandle pFH);
void InitTesting(void);
int StudentWriteDelegate(   FILE * fp, uint8_t * pData, 
                            unsigned long startLocation, unsigned long size);

