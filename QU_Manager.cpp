//
// Created by 唐艺峰 on 2018/12/20.
//

#include "QU_Manager.h"

void Init_Result(SelResult * res){
    res->next_res = nullptr;
}

void Destory_Result(SelResult * res){
    for(int i = 0;i<res->row_num;i++){
        for(int j = 0;j<res->col_num;j++){
            delete[] res->res[i][j];
        }
        delete[] res->res[i];
    }
    if(res->next_res != nullptr){
        Destory_Result(res->next_res);
    }
}

RC Query(char * sql,SelResult * res){
    return SUCCESS;
}

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult *res) {
    return SQL_SYNTAX;
}
