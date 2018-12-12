#include <iostream>
#include "RM_Manager.h"
#include "Bit_tools.h"

int main() {
    auto rm_fileHandle = new RM_FileHandle;
    char test[2000] = "hello world!";
    RID testID;

    RM_CreateFile((char *)"../test.db", 13 + 2000);
    RM_OpenFile((char *)"../test.db", rm_fileHandle);

    InsertRec(rm_fileHandle, test, &testID);

    RM_CloseFile(rm_fileHandle);

    printf("\n%ld", sizeof(bool) + sizeof(RID));
}