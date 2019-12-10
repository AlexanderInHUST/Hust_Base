#ifndef __QUERY_MANAGER_H_
#define __QUERY_MANAGER_H_
#include "str.h"
#include "Condition_tool.h"
#include "SYS_Manager.h"

#define MAX_LINES_NUM	10

typedef struct SelResult {
	int col_num;
	int row_num;
	AttrType type[20];	//结果集各字段的数据类型
	int length[20];		//结果集各字段值的长度
	char fields[20][20];//最多二十个字段名，而且每个字段的长度不超过20
	char ** res[MAX_LINES_NUM];	//最多一百条记录
	SelResult * next_res;
}SelResult;

typedef struct RIDSets {
	RID * rids;
	int * status;   // 0 -> death 1 -> inactive 2 -> active
	int num;
	int size;
}RIDSets;

typedef struct ResultsSets {
	bool * is_alive;
	char ** data;
	int num;
} ResultsSets;

void Init_Result(SelResult * res);
void Destory_Result(SelResult * res);

RC Query(char * sql, SelResult * res);

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult * res);

#endif