#include <iostream>
#include "RM_Manager.h"
#include "Bit_tools.h"

int main() {
    auto rm_fileHandle = new RM_FileHandle;
    char test_info[2000] = "Hello world!";
    char modi_info[2000] = "Goodbye world!";
    RID testID;
    auto fileScan = new RM_FileScan;

    RM_CreateFile((char *)"../test.db", 13 + 200);
    RM_OpenFile((char *)"../test.db", rm_fileHandle);
    OpenScan(fileScan, rm_fileHandle, 0, nullptr);

    for (int i = 0; i < 500; i++) {
        auto record = new RM_Record;
        RC result = GetNextRec(fileScan, record);
        if (result == SUCCESS) {
            printf("rec %d: %d %d %s\n", i, record->rid.pageNum, record->rid.slotNum, record->pData);
        } else {
            printf("rec %d: null\n",i);
        }
    }
    RM_CloseFile(rm_fileHandle);


//    auto rm_fileHandle = new RM_FileHandle;
//    char test_info[200] = "Hello world!";
//    char modi_info[200] = "Goodbye world!";
//    RID testID;
//
//    RM_CreateFile((char *)"../test.db", 13 + 200);
//    RM_OpenFile((char *)"../test.db", rm_fileHandle);
//
//    for (int i = 0; i < 200; i++) {
//        InsertRec(rm_fileHandle, test_info, &testID);
//    }
//
//    for (PageNum i = 2; i < 300; i++) {
//        for (SlotNum j = 0; j < 30; j++) {
//            RID tmpId;
//            tmpId.pageNum = i;
//            tmpId.slotNum = j;
//            auto newRecord = new RM_Record;
//            RC result = GetRec(rm_fileHandle, &tmpId, newRecord);
//            if (result == SUCCESS) {
//                printf("%d %d %s\n", i, j, newRecord->pData);
//            }
//        }
//    }
//    RM_CloseFile(rm_fileHandle);
}