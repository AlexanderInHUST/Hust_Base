//
// Created by 唐艺峰 on 2018/12/20.
//

#ifndef HUST_BASE_KERNEL_QU_MANAGER_H
#define HUST_BASE_KERNEL_QU_MANAGER_H

#include "str.h"
#include "Condition_tool.h"
#include "SYS_Manager.h"

typedef struct SelResult{
    int col_num;
    int row_num;
    AttrType type[20];	//结果集各字段的数据类型
    int length[20];		//结果集各字段值的长度
    char fields[20][20];//最多二十个字段名，而且每个字段的长度不超过20
    char ** res[100];	//最多一百条记录
    SelResult * next_res;
}SelResult;

typedef struct RIDSets {
    RID * rids;
    int * status;   // 0 -> death 1 -> inactive 2 -> active
    int num;
}RIDSets;

typedef struct ResultsSets {
    bool * is_alive;
    char ** data;
    int num;
} ResultsSets;

void Init_Result(SelResult * res);
void Destory_Result(SelResult * res);

RC Query(char * sql,SelResult * res);

RC Select(int nSelAttrs,RelAttr **selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res);

#endif //HUST_BASE_KERNEL_QU_MANAGER_H
