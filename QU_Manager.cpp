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

void DestroyColsInfo(char *** attrName, AttrType ** attrType, int ** attrLength, int ** attrOffset,
                     bool ** ixFlag, char *** indexName, int nRelations, const int * col_num) {
    for (int i = 0; i < nRelations; i++) {
        int cur_col_num = col_num[i];
        for (int j = 0; j < cur_col_num; j++) {
            delete attrName[i][j];
            delete indexName[i][j];
        }

        delete[] attrLength[i];
        delete[] attrOffset[i];
        delete[] attrType[i];
        delete[] ixFlag[i];
        delete[] attrName[i];
        delete[] indexName[i];
    }
}

void InitializeResultsSet(ResultsSets * set, int num, int value_len) {
    set->is_alive = new bool[num];
    set->data = new char * [num];
    set->num = num;
    for (int i = 0; i < num; i++) {
        set->data[i] = new char[value_len];
    }
}

void DestroyResultsSet(ResultsSets * set) {
    for (int i = 0; i < set->num; i++) {
        delete[] set->data[i];
    }
    delete[] set->is_alive;
    delete[] set->data;
}

void InitializeRidSet(RIDSets * set, int num) {
    set->rids = new RID[num];
    set->status = new int[num];
    set->num = num;
}

void InsertOneRid(RIDSets * set, int pos, RID aim) {
    set->rids[pos] = aim;
    set->status[pos] = 2;
}

void MakeAllInactive(RIDSets * set) {
    for (int i = 0; i < set->num; i++) {
        set->status[i] = 1;
    }
}

void MakeAllClear(RIDSets * set) {
    for (int i = 0; i < set->num; i++) {
        if (set->status[i] == 1) {
            set->status[i] = 0;
            set->num--;
        }
    }
}

bool MakeOneActive(RIDSets * set, RID aim) {
    for (int i = 0; i < set->num; i++) {
        if (set->status[i] == 0) {
            continue;
        }
        auto cur_rid = &set->rids[i];
        if (cur_rid->slotNum == aim.slotNum && cur_rid->pageNum == aim.pageNum) {
            set->status[i] = 2;
            return true;
        }
    }
    return false;
}

void DestoryRIDSets(RIDSets * set) {
    delete[] set->rids;
    delete[] set->status;
}

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult *res) {
    auto can_be_reduced = new char[nConditions]; // 0 -> nothing 1 -> reduced 2 -> deleted
    auto has_be_loaded = new bool[nRelations];

    auto col_num = new int[nRelations];
    auto col_name = new char ** [nRelations];
    auto col_length = new int * [nRelations];
    auto col_offset = new int * [nRelations];
    auto col_types = new AttrType * [nRelations];
    auto col_is_indx = new bool * [nRelations];
    auto col_indx_name = new char ** [nRelations];

    auto rid_sets = new RIDSets[nRelations];
    auto table_offset = new int[nRelations];
    auto table_value_num = new int[nRelations];

    for (int i = 0; i < nRelations; i++) {
        RC table_exist = GetTableInfo(relations[i], &col_num[i]);
        int cur_col_num = col_num[i];
        if (table_exist == TABLE_NOT_EXIST) {
            return TABLE_NOT_EXIST;
        }

        col_name[i] = new char * [cur_col_num];
        col_length[i] = new int[cur_col_num];
        col_offset[i] = new int[cur_col_num];
        col_types[i] = new AttrType[cur_col_num];
        col_is_indx[i] = new bool[cur_col_num];
        col_indx_name[i] = new char * [cur_col_num];
        for (int j = 0; j < cur_col_num; j++) {
            col_name[i][j] = new char[255];
            col_indx_name[i][j] = new char[255];
        }
        GetColsInfo(relations[i], col_name[i], col_types[i], col_length[i], col_offset[i], col_is_indx[i], col_indx_name[i]);
    }

    for (int i = 0; i < nConditions; i++) {
        auto cur_con = conditions[i];
        if (cur_con.bRhsIsAttr == 0 && cur_con.bLhsIsAttr == 0) {
            can_be_reduced[i] = 2;
        } else if (!(cur_con.bRhsIsAttr == 1 && cur_con.bLhsIsAttr == 1)) {
            can_be_reduced[i] = 1;
        } else {
            can_be_reduced[i] = 0;
            continue;
        } // to check whether this condition could be reduced

        if (can_be_reduced[i] == 1) {
            char * attr_name, * rel_name;
            AttrType value_type;
            char * value;

            if (cur_con.bLhsIsAttr == 1) {
                rel_name = cur_con.lhsAttr.relName;
                attr_name = cur_con.lhsAttr.attrName;

                value_type = cur_con.rhsValue.type;
                value = (char *) cur_con.rhsValue.data;
            } else {
                rel_name = cur_con.rhsAttr.relName;
                attr_name = cur_con.rhsAttr.attrName;

                value_type = cur_con.lhsValue.type;
                value = (char *) cur_con.lhsValue.data;
            } // initialize important data

            bool is_found = false;
            bool is_duplicate = false;
            bool is_done = false;
            int aim_attr_pos = 0, aim_relation_pos = 0;
            for (int j = 0; j < nRelations; j++) {
                for (int col_pos = 0; col_pos < col_num[j]; col_pos++) {
                    if (strcmp(attr_name, col_name[j][col_pos]) == 0) {
                        if (strcmp(rel_name, relations[j]) == 0) {
                            aim_relation_pos = j;
                            aim_attr_pos = col_pos;
                            is_done = true;
                            break;
                        }
                        is_duplicate = is_found;
                        is_found = true;
                        if (!is_duplicate) {
                            aim_relation_pos = j;
                            aim_attr_pos = col_pos;
                        }
                    }
                }
                if (is_done) {
                    break;
                }
            }
            if (!is_done && is_duplicate) {
                return SQL_SYNTAX;  // cannot tell which attr it is.
            } // to find where is the attr

            if (value_type != col_types[aim_relation_pos][aim_attr_pos]) {
                return RM_INVALIDRECSIZE;
            } // if they cannot be compared

            if (col_is_indx[aim_relation_pos][aim_attr_pos]) {
                auto cur_indx_name = col_indx_name[aim_relation_pos][aim_attr_pos];
                char full_index_name[255];
                strcpy(full_index_name, sys_dbpath);
                strcat(full_index_name, "/");
                strcat(full_index_name, sys_dbname);
                strcat(full_index_name, ".ix.");
                strcat(full_index_name, relations[aim_relation_pos]);
                strcat(full_index_name, ".");
                strcat(full_index_name, cur_indx_name);

                auto ix_indexHandle = new IX_IndexHandle;
                auto ix_scan = new IX_IndexScan;
                OpenIndex(full_index_name, ix_indexHandle);

                OpenIndexScan(ix_scan, ix_indexHandle, cur_con.op, value);
                RID tmp_rid;
                int rid_count = 0;
                if (!has_be_loaded[aim_relation_pos]) {
                    while (true) {
                        int isExist = IX_GetNextEntry(ix_scan, &tmp_rid);
                        if (isExist == INDEX_NOT_EXIST) {
                            break;
                        }
                        rid_count++;
                    }
                    CloseIndexScan(ix_scan); // to find the number of rids
                    InitializeRidSet(&rid_sets[aim_relation_pos], rid_count);

                    rid_count = 0;
                    OpenIndexScan(ix_scan, ix_indexHandle, cur_con.op, value);
                    while (true) {
                        int isExist = IX_GetNextEntry(ix_scan, &tmp_rid);
                        if (isExist == INDEX_NOT_EXIST) {
                            break;
                        }
                        InsertOneRid(&rid_sets[aim_relation_pos], rid_count, tmp_rid);
                        rid_count++;
                    }
                } else {
                    MakeAllInactive(&rid_sets[aim_relation_pos]);
                    while (true) {
                        int isExist = IX_GetNextEntry(ix_scan, &tmp_rid);
                        if (isExist == INDEX_NOT_EXIST) {
                            break;
                        }
                        MakeOneActive(&rid_sets[aim_relation_pos], tmp_rid);
                    }
                    MakeAllClear(&rid_sets[aim_relation_pos]);
                }
                CloseIndexScan(ix_scan); // copy all rids
                CloseIndex(ix_indexHandle);
                delete ix_scan;
                delete ix_indexHandle;
            } else {
                char full_tab_name[255];
                strcpy(full_tab_name, sys_dbpath);
                strcat(full_tab_name, "/");
                strcat(full_tab_name, sys_dbname);
                strcat(full_tab_name, ".tb.");
                strcat(full_tab_name, relations[aim_relation_pos]);

                auto converted_con = convert_conditions(1, &cur_con, col_num[aim_relation_pos], col_name[aim_relation_pos],
                        col_length[aim_relation_pos], col_offset[aim_relation_pos], col_types[aim_relation_pos]);

                RM_FileHandle rm_fileHandle;
                RM_FileScan rm_fileScan;
                RM_OpenFile(full_tab_name, &rm_fileHandle);

                OpenScan(&rm_fileScan, &rm_fileHandle, 1, converted_con);
                RM_Record record;
                int rid_count = 0;

                if (!has_be_loaded[aim_relation_pos]) {
                    while (true) {
                        RC result = GetNextRec(&rm_fileScan, &record);
                        if (result != SUCCESS)  {
                            break;
                        }
                        rid_count++;
                    }
                    CloseScan(&rm_fileScan);
                    InitializeRidSet(&rid_sets[aim_relation_pos], rid_count);

                    rid_count = 0;
                    OpenScan(&rm_fileScan, &rm_fileHandle, 1, converted_con);
                    while (true) {
                        RC result = GetNextRec(&rm_fileScan, &record);
                        if (result != SUCCESS)  {
                            break;
                        }
                        InsertOneRid(&rid_sets[aim_relation_pos], rid_count, record.rid);
                        rid_count++;
                    }
                } else {
                    MakeAllInactive(&rid_sets[aim_relation_pos]);
                    while (true) {
                        int isExist = GetNextRec(&rm_fileScan, &record);
                        if (isExist != SUCCESS) {
                            break;
                        }
                        MakeOneActive(&rid_sets[aim_relation_pos], record.rid);
                    }
                    MakeAllClear(&rid_sets[aim_relation_pos]);
                }
                CloseScan(&rm_fileScan);
                RM_CloseFile(&rm_fileHandle);
            } // done rid collections

            has_be_loaded[aim_relation_pos] = true; // this relation has been loadeds
        } else {
            auto converted_con = convert_conditions(1, &cur_con, 0, nullptr, nullptr, nullptr, nullptr);
            RM_FileScan dump_scan;
            dump_scan.conditions = converted_con;
            dump_scan.conNum = 1;
            if (checkConditions(&dump_scan, nullptr)) {
                continue;
            } else {
                return SUCCESS;
            }
        }
    } // reduced all possible relations

    for (int i = 0; i < nRelations; i++) {
        if (!has_be_loaded[i]) {
            char full_tab_name[255];
            strcpy(full_tab_name, sys_dbpath);
            strcat(full_tab_name, "/");
            strcat(full_tab_name, sys_dbname);
            strcat(full_tab_name, ".tb.");
            strcat(full_tab_name, relations[i]);

            RM_FileHandle rm_fileHandle;
            RM_FileScan rm_fileScan;
            RM_OpenFile(full_tab_name, &rm_fileHandle);

            OpenScan(&rm_fileScan, &rm_fileHandle, 0, nullptr);
            RM_Record record;
            int rid_count = 0;
            while (true) {
                RC result = GetNextRec(&rm_fileScan, &record);
                if (result != SUCCESS)  {
                    break;
                }
                rid_count++;
            }
            CloseScan(&rm_fileScan);
            InitializeRidSet(&rid_sets[i], rid_count);

            rid_count = 0;
            OpenScan(&rm_fileScan, &rm_fileHandle, 0, nullptr);
            while (true) {
                RC result = GetNextRec(&rm_fileScan, &record);
                if (result != SUCCESS)  {
                    break;
                }
                InsertOneRid(&rid_sets[i], rid_count, record.rid);
                rid_count++;
            }
            CloseScan(&rm_fileScan);
            RM_CloseFile(&rm_fileHandle);
        }

        if (i == 0) {
            table_offset[i] = 0;
        } else {
            table_offset[i] = col_offset[i - 1][col_num[i - 1] - 1] + table_offset[i - 1];
        } // get table offset for Descartes
    } // get rest possible result

    for (int i = nRelations - 1; i >= 0; i--) {
        if (i == nRelations - 1) {
            table_value_num[i] = 1;
        } else {
            table_value_num[i] = rid_sets[i + 1].num * table_value_num[i + 1];
        }
    } // get the number of each value

    ResultsSets resultsSets;
    InitializeResultsSet(&resultsSets, table_value_num[0] * rid_sets[0].num, table_offset[nRelations - 1]);
    for (int i = 0; i < nRelations; i++) {
        int line_num = 0;
        char full_tab_name[255];
        strcpy(full_tab_name, sys_dbpath);
        strcat(full_tab_name, "/");
        strcat(full_tab_name, sys_dbname);
        strcat(full_tab_name, ".tb.");
        strcat(full_tab_name, relations[i]);
        RM_FileHandle rm_fileHandle;
        RM_OpenFile(full_tab_name, &rm_fileHandle);

        for (int j = 0; j < rid_sets[i].num; j++) {
            auto cur_rid = rid_sets[i].rids[j];
            RM_Record cur_record;
            GetRec(&rm_fileHandle, &cur_rid, &cur_record);
            for (int k = 0; k < table_value_num[i]; k++) {
                memcpy(resultsSets.data[line_num] + table_offset[i], cur_record.pData, (size_t) col_offset[i][col_num[i] - 1]);
                line_num++;
            }
        }
        RM_CloseFile(&rm_fileHandle);
    } // make up the whole set

    RM_Record dump_record;
    for (int i = 0; i < nConditions; i++) {
        if (can_be_reduced[i] == 0) {
            auto cur_con = conditions[i];
            


            for (int j = 0; j < resultsSets.num; j++) {
                if (resultsSets.is_alive[j]) {
                    dump_record.pData = resultsSets.data[j];
                    Con dump_con;
                    dump_con.bLhsIsAttr = 1;
                    dump_con.bRhsIsAttr = 1;
                }
            }
        }
    }

    DestroyColsInfo(col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name, nRelations, col_num);
    delete[] col_num;
    delete[] col_name;
    delete[] col_length;
    delete[] col_offset;
    delete[] col_types;
    delete[] col_is_indx;
    delete[] col_indx_name;
    return SQL_SYNTAX;
}
