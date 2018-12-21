//
// Created by 唐艺峰 on 2018/12/18.
//

#ifndef HUST_BASE_KERNEL_SYS_MANAGER_H
#define HUST_BASE_KERNEL_SYS_MANAGER_H

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "str.h"

extern char sys_dbname[255];
extern int sys_table_fd;
extern int sys_col_fd;
extern int sys_table_row_num;
extern int sys_col_row_num;
extern char * sys_table_data;
extern char * sys_col_data;
extern RID cur_rid;

#define TABLENAME_SIZE  21
#define ATTRCOUNT_SIZE  4
#define ATTRNAME_SIZE   21
#define ATTRTYPE_SIZE   4
#define ATTRLENGTH_SIZE 4
#define ATTROFFSET_SIZE 4
#define IX_FLAG_SIZE    1
#define INDEXNAME_SIZE  21

#define IX_FLAG_OFFSET  ((size_t) (21 + 21 + 4 + 4 + 4))

#define MAX_FILE_SIZE   65535

#define TABLE_ROW_SIZE  ((size_t) (21 + 4))
#define COL_ROW_SIZE    ((size_t) (21 + 21 + 4 + 4 + 4 + 1 + 21))

//void ExecuteAndMessage(char * ,CEditArea*);
//bool CanButtonClick();

RC CreateDB(char *dbpath, char *dbname);

RC DropDB(char *dbname);

RC OpenDB(char *dbname);

RC CloseDB();

RC execute(char *sql);

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes);

RC DropTable(char *relName);

RC CreateIndex(char *indexName, char *relName, char *attrName);

RC DropIndex(char *indexName);

RC Insert(char *relName, int nValues, Value *values);

RC Delete(char *relName, int nConditions, Condition *conditions);

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions);

RC GetColsInfo(char *relName, int colNum, char ** attrName, AttrType * attrType, int * attrLength, int * attrOffset,
               bool * ixFlag, char ** indexName);

RC GetTableInfo(char *relName, int *colNum);

#endif //HUST_BASE_KERNEL_SYS_MANAGER_H
