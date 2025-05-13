#include <stdio.h>
#include "ImageFile.h"
#include "Student.h"

int main(void)
{
    InitTesting();

    TestFile(PIC_C);
    TestFile(PIC_M);
    TestFile(PIC_O);
    TestFile(PIC_P);
}