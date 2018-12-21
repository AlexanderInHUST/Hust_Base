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

//RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult *res) {
//    auto results_set = new SelResult[nConditions];
//    bool has_be_loaded[nRelations];
//    bool can_be_reduced[nConditions];
//    bool can_be_deleted[nConditions];
//
//    int col_num[nRelations];
//    char ** col_name[nRelations];
//    int * col_length[nRelations];
//    int * col_offset[nRelations];
//    AttrType * col_types[nRelations];
//    bool * col_is_indx[nRelations];
//    char ** col_indx_name[nRelations];
//
//    for (int i = 0; i < nRelations; i++) {
//        RC table_exist = GetTableInfo(relations[i], &col_num[i]);
//        int cur_col_num = col_num[i];
//        if (table_exist == TABLE_NOT_EXIST) {
//            return TABLE_NOT_EXIST;
//        }
//
//        col_name[i] = new char * [cur_col_num];
//        col_length[i] = new int[cur_col_num];
//        col_offset[i] = new int[cur_col_num];
//        col_types[i] = new AttrType[cur_col_num];
//        col_is_indx[i] = new bool[cur_col_num];
//        col_indx_name[i] = new char * [cur_col_num];
//        for (int j = 0; j < cur_col_num; j++) {
//            col_name[i][j] = new char[255];
//            col_indx_name[i][j] = new char[255];
//        }
//
//        GetColsInfo(relations[i], cur_col_num, col_name[i], col_types[i], col_length[i], col_offset[i], col_is_indx[i], col_indx_name[i]);
//    }
//
//    for (int i = 0; i < nConditions; i++) {
//        auto cur_con = conditions[i];
//        can_be_reduced[i] = !(cur_con.bRhsIsAttr == 1 && cur_con.bLhsIsAttr == 1);
//        can_be_deleted[i] = (cur_con.bRhsIsAttr == 0 && cur_con.bLhsIsAttr == 0);
//
//        if (can_be_reduced[i] && !can_be_deleted[i]) {
//            Init_Result(&results_set[i]);
//            char * attr_name, * rel_name;
//            AttrType value_type;
//            char * value;
//
//            if (cur_con.bLhsIsAttr == 1) {
//                rel_name = cur_con.lhsAttr.relName;
//                attr_name = cur_con.lhsAttr.attrName;
//
//                value_type = cur_con.rhsValue.type;
//                value = (char *) cur_con.rhsValue.data;
//            } else {
//                rel_name = cur_con.rhsAttr.relName;
//                attr_name = cur_con.rhsAttr.attrName;
//
//                value_type = cur_con.lhsValue.type;
//                value = (char *) cur_con.lhsValue.data;
//            }
//
//            bool is_found = false;
//            for (int j = 0; j < nRelations; j++) {
////                if (strcmp(attr_name, col_name[i]) == 0) {
//
////                }
//            }
//        }
//    }
//
//    return SQL_SYNTAX;
//}
