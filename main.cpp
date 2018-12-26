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
//    RID rid[500];
//
//    for (int i = 0; i < 500; i++) {
//        InsertRec(rm_fileHandle, test_info, &rid[i]);
//    }
//
//    for (int i = 0; i < 250; i++) {
//        DeleteRec(rm_fileHandle, &rid[i]);
//    }
//
//    for (int i = 0; i < 250; i++) {
//        InsertRec(rm_fileHandle, modi_info, &rid[i]);
//    }
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
//    for (int i = 0; i < 1000; i++) {
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
    int testData = 42;

    for (int i = 0; i < 50; i++) {
        rid.pageNum = i;
        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
    }

    for (int i = 0; i < 50; i++) {
        rid.pageNum = i;
        printf("?? %d\n", i);
        if (i == 45) {
            printf("????\n");
        }
        DeleteEntry(ix_indexHandle, (char *) &testData, &rid);
    }
    printf("?\n");
//
//    testData = 43;
//    for (int i = 40; i < 50; i++) {
//        rid.pageNum = i;
//        InsertEntry(ix_indexHandle, (char *) &testData, &rid);
//    }
//    printf("?\n");

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
    traverseAll(ix_indexHandle);
    CloseIndex(ix_indexHandle);

    auto tree = new Tree;
    GetIndexTree((char *) "../test.ix", tree);

//    CreateDB((char *)"..", (char *)"test");
//    OpenDB((char *) "test");
//
//    auto attributes = new AttrInfo[5];
//    for (int i = 0; i < 5; i++) {
//        auto cur_attr = &attributes[i];
//        cur_attr->attrType = ints;
//        cur_attr->attrLength = 4;
//        cur_attr->attrName = new char[21];
//        char tmp[10];
//        sprintf(tmp, "%d", i);
//        strcpy(cur_attr->attrName, (char *) "hello");
//        strcat(cur_attr->attrName, tmp);
//    }
//    CreateTable((char *) "hello", 5, attributes);
//
//    attributes = new AttrInfo[6];
//    for (int i = 0; i < 6; i++) {
//        auto cur_attr = &attributes[i];
//        cur_attr->attrType = chars;
//        cur_attr->attrLength = 4;
//        cur_attr->attrName = new char[21];
//        char tmp[10];
//        sprintf(tmp, "%d", i);
//        strcpy(cur_attr->attrName, (char *) "bye");
//        strcat(cur_attr->attrName, tmp);
//    }
//    CreateTable((char *) "bye", 6, attributes);
//
//    attributes = new AttrInfo[6];
//    for (int i = 0; i < 6; i++) {
//        auto cur_attr = &attributes[i];
//        cur_attr->attrType = chars;
//        cur_attr->attrLength = 4;
//        cur_attr->attrName = new char[21];
//        char tmp[10];
//        sprintf(tmp, "%d", i);
//        strcpy(cur_attr->attrName, (char *) "nice");
//        strcat(cur_attr->attrName, tmp);
//    }
//    CreateTable((char *) "nice", 6, attributes);
//
//    CreateIndex("hello_world", "hello", "hello0");
//    CreateIndex("hello_world1", "hello", "hello2");
//    CreateIndex("hello_world2", "bye", "bye1");
//
//    Value test_values[5];
//    for (int i = 0; i < 5; i++) {
//        test_values[i].type = ints;
//        test_values[i].data = new char[4];
//        memcpy(test_values[i].data, &i, sizeof(int));
//    }
//
//    for (int i = 0; i < 500; i++) {
//        memcpy(test_values[0].data, &i, sizeof(int));
//        Insert("hello", 5, test_values);
//    }
//
//    char full_index_name[255];
//    strcpy(full_index_name, sys_dbpath);
//    strcat(full_index_name, "/");
//    strcat(full_index_name, sys_dbname);
//    strcat(full_index_name, ".ix.");
//    strcat(full_index_name, "hello");
//    strcat(full_index_name, ".");
//    strcat(full_index_name, "hello_world1");
//
//    int tmp = 200;
//    char tmp_char[4];
//    memcpy(tmp_char, &tmp, sizeof(int));
//    auto cons = new Condition[1];
//    cons->op = GreatT;
//    cons->bLhsIsAttr = 1;
//    cons->bRhsIsAttr = 0;
//    cons->lhsAttr.attrName = new char[21];
//    strcpy(cons->lhsAttr.attrName, "hello0");
//    cons->rhsValue.type = ints;
//    cons->rhsValue.data = tmp_char;
//
//    int tmp_up = 300;
//    char tmp_up_char[4];
//    memcpy(tmp_up_char, &tmp_up, sizeof(int));
//    Value tmp_value;
//    tmp_value.type = ints;
//    tmp_value.data = tmp_up_char;
//    Update("hello", "hello0", &tmp_value, 1, cons);
//
////    Delete("hello", 1, cons);
////    for (int i = 1001; i < 5000; i++) {
////        memcpy(test_values[0].data, &i, sizeof(int));
////        Insert("hello", 5, test_values);
////    }
//
////    Delete("hello", 0, nullptr);
//
//    auto index_handle = new IX_IndexHandle;
//    OpenIndex(full_index_name, index_handle);
//    traverseAll(index_handle);
//    CloseIndex(index_handle);
//
////
//
////
//////    Delete("hello", 1, cons);
////
////
////    DropIndex("hello_world");
//
////    DropTable((char *) "bye");
////    DropTable((char *) "nice");
////    DropTable((char *) "hello");
////    CloseDB();
////    DropDB((char *) "../test");
}