#include <iostream>
#include "RM_Manager.h"
#include "IX_Manager.h"
#include "Bit_tools.h"
#include "SYS_Manager.h"

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

//    auto ix_indexHandle = new IX_IndexHandle;
//    CreateIndex("../test.ix", ints, sizeof(int));
//    OpenIndex("../test.ix", ix_indexHandle);
//
//    RID rid;
//    rid.bValid = true;
//    rid.slotNum = 1;
//    int testData = 42;
//
//    for (int i = 20; i < 40; i++) {
//        rid.pageNum = i;
//        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//
//    testData = 43;
//    for (int i = 40; i < 50; i++) {
//        rid.pageNum = i;
//        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//
//    int tmp = 43;
//    auto indexScan = new IX_IndexScan;
//    OpenIndexScan(indexScan, ix_indexHandle, GEqual, (char *) &tmp);
//    while (true) {
//        RID tmpRid;
//        int isExist = IX_GetNextEntry(indexScan, &tmpRid);
//        if (isExist == INDEX_NOT_EXIST) {
//            break;
//        }
//        printf("rid: %d %d\n", tmpRid.pageNum, tmpRid.slotNum);
//    }
//    traverseAll(ix_indexHandle);
//    CloseIndex(ix_indexHandle);
//
//    auto tree = new Tree;
//    GetIndexTree((char *) "../test.ix", tree);

    CreateDB((char *)"..", (char *)"test");
    OpenDB((char *) "../test");

    auto attributes = new AttrInfo[5];
    for (int i = 0; i < 5; i++) {
        auto cur_attr = &attributes[i];
        cur_attr->attrType = ints;
        cur_attr->attrLength = 4;
        cur_attr->attrName = new char[21];
        char tmp[10];
        sprintf(tmp, "%d", i);
        strcpy(cur_attr->attrName, (char *) "hello");
        strcat(cur_attr->attrName, tmp);
    }
//    CreateTable((char *) "hello", 5, attributes);

    attributes = new AttrInfo[6];
    for (int i = 0; i < 6; i++) {
        auto cur_attr = &attributes[i];
        cur_attr->attrType = chars;
        cur_attr->attrLength = 4;
        cur_attr->attrName = new char[21];
        char tmp[10];
        sprintf(tmp, "%d", i);
        strcpy(cur_attr->attrName, (char *) "bye");
        strcat(cur_attr->attrName, tmp);
    }
//    CreateTable((char *) "bye", 6, attributes);

    Value test_values[5];
    for (int i = 0; i < 5; i++) {
        test_values[i].type = ints;
        test_values[i].data = new char[4];
        memcpy(test_values[i].data, &i, sizeof(int));
    }

    for (int i = 0; i < 10; i++) {
        Insert("hello", 5, test_values);
    }


    Delete("hello", 0, nullptr);


//    CreateIndex("hello_world", "hello", "hello1");
//    DropIndex("hello_world");
//    DropTable((char *) "hello");
//    DropTable((char *) "bye");
    CloseDB();
//    DropDB((char *) "../test");
}