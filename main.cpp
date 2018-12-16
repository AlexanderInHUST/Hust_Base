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

    RID rid;
    rid.bValid = true;
    rid.slotNum = 1;
    int testData = rid.pageNum = 5;

    testData = rid.pageNum = 22;
    DeleteEntry(ix_indexHandle, (char *) &testData, &rid);

    testData = rid.pageNum = 15;
    DeleteEntry(ix_indexHandle, (char *) &testData, &rid);

//    testData = rid.pageNum = 7;
//    DeleteEntry(ix_indexHandle, (char *) &testData, &rid);

//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 8;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 10;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 15;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 16;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 17;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 18;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//
//    testData = rid.pageNum = 6;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 9;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 19;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 20;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 21;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    testData = rid.pageNum = 22;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//
//    testData = rid.pageNum = 7;
//    InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//
    traverseAll(ix_indexHandle);
    CloseIndex(ix_indexHandle);



    auto tree = new Tree;
    GetIndexTree((char *) "../test.ix", tree);
    printf("123");
}