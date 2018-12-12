#include <iostream>
#include "RM_Manager.h"
#include "Bit_tools.h"

int main() {
    auto rm_fileHandle = new RM_FileHandle;
    char test[2000] = "Goodbye world!";

    RID testID;
    testID.pageNum = 3;
    testID.slotNum = 0;
    auto record = new RM_Record;
    record->rid = testID;
    record->pData = test;

    auto newRecord = new RM_Record;


    RM_CreateFile((char *)"../test.db", 13 + 2000);
    RM_OpenFile((char *)"../test.db", rm_fileHandle);

    UpdateRec(rm_fileHandle, record);

    GetRec(rm_fileHandle, &testID, newRecord);
//
//    InsertRec(rm_fileHandle, test, &testID);

    RM_CloseFile(rm_fileHandle);

    printf("%s\n", newRecord->pData);
}