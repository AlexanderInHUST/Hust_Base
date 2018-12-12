#include <iostream>
#include "RM_Manager.h"
#include "Bit_tools.h"

int main() {
    auto rm_fileHandle = new RM_FileHandle;
    char test[2000] = "hello world!";

    RID testID;
    testID.pageNum = 2;
    testID.slotNum = 3;
    auto record = new RM_Record;

    RM_CreateFile((char *)"../test.db", 13 + 2000);
    RM_OpenFile((char *)"../test.db", rm_fileHandle);

    GetRec(rm_fileHandle, &testID, record);
//
//    InsertRec(rm_fileHandle, test, &testID);

    RM_CloseFile(rm_fileHandle);

    printf("%s\n", record->pData);
}