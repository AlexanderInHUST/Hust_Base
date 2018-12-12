#include <iostream>
#include "RM_Manager.h"
#include "Bit_tools.h"

int main() {
    auto rm_fileHandle = new RM_FileHandle;
    char test_info[2000] = "Hello world!";
    char modi_info[2000] = "Goodbye world!";
    RID testID;

    RM_CreateFile((char *)"../test.db", 13 + 2000);
    RM_OpenFile((char *)"../test.db", rm_fileHandle);


    for (int i = 0; i < 10; i++) {
        InsertRec(rm_fileHandle, test_info, &testID);
    }

//    for (PageNum i = 2; i < 7; i++) {
//        RID tmpId;
//        tmpId.pageNum = i;
//        tmpId.slotNum = 0;
//        auto newRecord = new RM_Record;
//        newRecord->rid = tmpId;
//        newRecord->pData = modi_info;
//
//        UpdateRec(rm_fileHandle, newRecord);
//    }

    for (PageNum i = 2; i < 4; i++) {
        RID tmpId;
        tmpId.pageNum = i;
        tmpId.slotNum = 1;

        DeleteRec(rm_fileHandle, &tmpId);
    }
////
//    for (int i = 0; i < 10; i++) {
//        InsertRec(rm_fileHandle, test_info, &testID);
//    }


//    for (PageNum i = 2; i < 7; i++) {
//        for (SlotNum j = 0; j < 1; j++) {
//            RID tmpId;
//            tmpId.pageNum = i;
//            tmpId.slotNum = j;
//            auto newRecord = new RM_Record;
//            GetRec(rm_fileHandle, &tmpId, newRecord);
//            printf("%d %d %s\n", i, j, newRecord->pData);
//        }
//    }

    RM_CloseFile(rm_fileHandle);
}