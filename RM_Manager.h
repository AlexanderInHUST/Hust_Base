//
// Created by 唐艺峰 on 2018/12/11.
//

#ifndef HUST_BASE_KERNEL_RM_MANAGER_H
#define HUST_BASE_KERNEL_RM_MANAGER_H

#include "PF_Manager.h"
#include "Bit_tools.h"
#include "math.h"
#include "str.h"

typedef int SlotNum;

typedef struct {
    PageNum pageNum;    //记录所在页的页号
    SlotNum slotNum;        //记录的插槽号
    bool bValid;            //true表示为一个有效记录的标识符
} RID;

typedef struct {
    bool bValid;         // False表示还未被读入记录
    RID rid;             // 记录的标识符
} RM_Record;

// Above is the detail for record

typedef struct {
    int nRecords;			//当前文件中包含的记录数
    int recordSize;			//每个记录的大小 (包含bool及标识符，以char个数计算)
    int recordsPerPage;		//每个页面可以装载的记录数量
} RM_FileSubHeader;

//typedef struct {
//    int recordsNum;
//    char *recordsMap;
//    RM_Record *records;
//} RM_Page;

typedef struct {//文件句柄
    bool bOpen;//句柄是否打开（是否正在被使用）
    PF_FileHandle *pf_fileHandle;
    RM_FileSubHeader *rm_fileSubHeader;
    char *header_bitmap;
} RM_FileHandle;

// Above is the detail for files

typedef struct {
    int bLhsIsAttr, bRhsIsAttr;//左、右是属性（1）还是值（0）
    AttrType attrType;
    int LattrLength, RattrLength;
    int LattrOffset, RattrOffset;
    CompOp compOp;
    void *Lvalue, *Rvalue;
} Con;

typedef struct {
    bool bOpen;        //扫描是否打开
    RM_FileHandle *pRMFileHandle;        //扫描的记录文件句柄
    int conNum;        //扫描涉及的条件数量
    Con *conditions;    //扫描涉及的条件数组指针
    PF_PageHandle PageHandle; //处理中的页面句柄
    PageNum pn;    //扫描即将处理的页面号
    SlotNum sn;        //扫描即将处理的插槽号
} RM_FileScan;

RC GetNextRec(RM_FileScan *rmFileScan, RM_Record *rec);

RC OpenScan(RM_FileScan *rmFileScan, RM_FileHandle *fileHandle, int conNum, Con *conditions);

RC CloseScan(RM_FileScan *rmFileScan);

RC UpdateRec(RM_FileHandle *fileHandle, const RM_Record *rec);

RC DeleteRec(RM_FileHandle *fileHandle, const RID *rid);

RC InsertRec(RM_FileHandle *fileHandle, char *pData, RID *rid);

RC GetRec(RM_FileHandle *fileHandle, RID *rid, RM_Record *rec);

RC RM_CloseFile(RM_FileHandle *fileHandle);

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle);

RC RM_CreateFile(char *fileName, int recordSize);

#endif //HUST_BASE_KERNEL_RM_MANAGER_H
