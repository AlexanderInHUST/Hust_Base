//
// Created by 唐艺峰 on 2018/12/19.
//

#include "Condition_tool.h"

Con *convert_conditions(int con_num, Condition *cons, int col_num, char col_name[][255], const int *col_length, const int *col_offset, const AttrType *col_types) {
    auto converted_cons = new Con[con_num];
    for (int i = 0; i < con_num; i++) {
        auto cur_con = cons[i];
        auto cur_converted = &converted_cons[i];
        char * left_value = nullptr, * right_value = nullptr;
        int left_length = 0, right_length = 0;
        int left_offset = 0, right_offset = 0;
        AttrType attr_type = ints;

        if (cur_con.bLhsIsAttr == 0) {
            attr_type = cur_con.lhsValue.type;
            left_value = (char *) cur_con.lhsValue.data;
        } else {
            for (int j = 0; j < col_num; j++) {
                if (strcmp(cur_con.lhsAttr.attrName, col_name[j]) == 0) {
                    attr_type = col_types[j];
                    left_length = col_length[j];
                    left_offset = col_offset[j];
                    break;
                }
            }
        }

        if (cur_con.bRhsIsAttr == 0) {
            attr_type = cur_con.rhsValue.type;
            right_value = (char *) cur_con.rhsValue.data;
        } else {
            for (int j = 0; j < col_num; j++) {
                if (strcmp(cur_con.rhsAttr.attrName, col_name[j]) == 0) {
                    attr_type = col_types[j];
                    right_length = col_length[j];
                    right_offset = col_offset[j];
                }
            }
        }

        cur_converted->bLhsIsAttr = cur_con.bLhsIsAttr;
        cur_converted->bRhsIsAttr = cur_con.bRhsIsAttr;
        cur_converted->attrType = attr_type;
        cur_converted->LattrLength = left_length;
        cur_converted->LattrOffset = left_offset;
        cur_converted->RattrLength = right_length;
        cur_converted->RattrOffset = right_offset;
        cur_converted->compOp = cur_con.op;
        cur_converted->Lvalue = left_value;
        cur_converted->Rvalue = right_value;
    }
    return converted_cons;
}