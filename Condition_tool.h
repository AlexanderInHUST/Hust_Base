#pragma once

#include "str.h"
#include "RM_Manager.h"

Con *convert_conditions(int con_num, Condition *cons, int col_num, char ** col_name, const int *col_length, const int *col_offset, const AttrType *col_types);
