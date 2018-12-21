//
// Created by 唐艺峰 on 2018/12/19.
//

#ifndef HUST_BASE_KERNEL_CONDITION_TOOL_H
#define HUST_BASE_KERNEL_CONDITION_TOOL_H

#include "str.h"
#include "RM_Manager.h"

Con *convert_conditions(int con_num, Condition *cons, int col_num, char ** col_name, const int *col_length, const int *col_offset, const AttrType *col_types);

#endif //HUST_BASE_KERNEL_CONDITION_TOOL_H
