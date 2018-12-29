//
// Created by 唐艺峰 on 2018/12/13.
//

#ifndef HUST_BASE_KERNEL_IX_MANAGER_H
#define HUST_BASE_KERNEL_IX_MANAGER_H

#include "PF_Manager.h"
#include "RM_Manager.h"

typedef struct {
    int attrLength;
    int keyLength;
    AttrType attrType;
    PageNum rootPage;
    PageNum first_leaf;
    int order;
} IX_FileHeader;

typedef struct {
    int is_leaf;
    int keynum;
    PageNum parent;
    PageNum brother;
//    char *keys;          // keys                            fix
//    char *rids;          // children (keynum + 1) or rids   fix
    int keys_offset;    // fix
    int rids_offset;    // fix
} IX_Node;

// Above is node control msg

typedef struct {
    bool bOpen;
    PF_FileHandle * fileHandle; // fix here
    IX_FileHeader * fileHeader; // fix here
    PF_PageHandle * headerPage;
} IX_IndexHandle;

typedef struct {
    bool bOpen;                     /*扫描是否打开 */
    IX_IndexHandle *pIXIndexHandle; //指向索引文件操作的指针
    CompOp compOp;                  /* 用于比较的操作符*/
    char *value;                    /* 与属性行比较的值 */
//    PF_PageHandle
//    pfPageHandles[PF_BUFFER_SIZE]; // 固定在缓冲区页面所对应的页面操作列表
    PageNum pnNext;                    //下一个将要被读入的页面号
    int ridIx;
} IX_IndexScan;

typedef struct Tree_Node {
    int keyNum;            //节点中包含的关键字（属性值）个数
    char **keys;           //节点中包含的关键字（属性值）数组
    Tree_Node *parent;     //父节点
    Tree_Node *sibling;    //右边的兄弟节点
    Tree_Node *firstChild; //最左边的孩子节点
} Tree_Node;             //节点数据结构

typedef struct {
    AttrType attrType; // B+树对应属性的数据类型
    int attrLength;    // B+树对应属性值的长度
    int order;         // B+树的序数
    Tree_Node *root;   // B+树的根节点
} Tree;

// Just for test

RC CreateIndex(const char *fileName, AttrType attrType, int attrLength);
RC OpenIndex(const char *fileName, IX_IndexHandle *indexHandle);
RC CloseIndex(IX_IndexHandle *indexHandle);

RC InsertEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid);
RC DeleteEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid, int flag);
RC OpenIndexScan(IX_IndexScan *indexScan, IX_IndexHandle *indexHandle,
                 CompOp compOp, char *value);
RC IX_GetNextEntry(IX_IndexScan *indexScan, RID *rid);
RC CloseIndexScan(IX_IndexScan *indexScan);
RC GetIndexTree(char *fileName, Tree *index);

void traverseAll(IX_IndexHandle *indexHandle);

#endif //HUST_BASE_KERNEL_IX_MANAGER_H
