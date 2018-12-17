#include <iostream>
#include "RM_Manager.h"
#include "IX_Manager.h"
#include "Bit_tools.h"

int main() {
//    auto rm_fileHandle = new RM_FileHandle;
//    char test_info[200] = "Hello world!";
//    char modi_info[200] = "Goodbye world!";
//    RID testID;
//    auto fileScan = new RM_FileScan;
//
//    RM_CreateFile((char *) "../test.db", 13 + 200);
//    RM_OpenFile((char *) "../test.db", rm_fileHandle);
//
//    auto tmpCon = new Con;
//    tmpCon->bLhsIsAttr = 1;
//    tmpCon->bRhsIsAttr = 0;
//    tmpCon->attrType = chars;
//    tmpCon->LattrLength = sizeof(char) * 200;
//    tmpCon->LattrOffset = 0;
//    tmpCon->compOp = GEqual;
//    tmpCon->Rvalue = modi_info;
//
//    OpenScan(fileScan, rm_fileHandle, 1, tmpCon);
//
//    for (int i = 0; i < 500; i++) {
//        auto record = new RM_Record;
//        RC result = GetNextRec(fileScan, record);
//
//        if (result == SUCCESS) {
//            printf("rec %d: %d %d %s\n", i, record->rid.pageNum, record->rid.slotNum, record->pData);
//        } else {
//            printf("rec %d: null\n", i);
//        }
//    }
//
//    RM_CloseFile(rm_fileHandle);

    auto ix_indexHandle = new IX_IndexHandle;
    CreateIndex("../test.ix", ints, sizeof(int));
    OpenIndex("../test.ix", ix_indexHandle);

//    RID rid;
//    rid.bValid = true;
//    rid.slotNum = 1;
//    int testData = rid.pageNum = 5;
//
//    for (int i = 0; i <= 50; i++) {
//        testData = rid.pageNum = i;
//        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//
//    for (int i = 0; i < 30; i++) {
//        testData = rid.pageNum = i;
//        DeleteEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//
//    for (int i = 0; i < 10; i++) {
//        testData = rid.pageNum = i;
//        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//
//    for (int i = 0; i < 5; i++) {
//        testData = rid.pageNum = i;
//        DeleteEntry(ix_indexHandle, (char *) &testData, &rid);
//    }

    int tmp = -1;
    auto indexScan = new IX_IndexScan;
    OpenIndexScan(indexScan, ix_indexHandle, GreatT, (char *) &tmp);
    while (true) {
        RID tmpRid;
        int isExist = IX_GetNextEntry(indexScan, &tmpRid);
        if (isExist == INDEX_NOT_EXIST) {
            break;
        }
        printf("rid: %d %d\n", tmpRid.pageNum, tmpRid.slotNum);
    }


    traverseAll(ix_indexHandle);
    CloseIndex(ix_indexHandle);



    auto tree = new Tree;
    GetIndexTree((char *) "../test.ix", tree);
    printf("123");
}