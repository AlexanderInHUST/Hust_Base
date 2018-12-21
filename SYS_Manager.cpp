//
// Created by 唐艺峰 on 2018/12/18.
//

#include "SYS_Manager.h"
#include "Condition_tool.h"

char sys_dbpath[255];
char sys_dbname[255];
RID cur_rid;
RM_FileHandle * table_file_handle;
RM_FileHandle * col_file_handle;

RC OpenSysTables() {
    char full_dbname[255];
    strcpy(full_dbname, sys_dbpath);
    strcat(full_dbname, "/SYSTABLES");
    RM_OpenFile(full_dbname, table_file_handle);
}

RC OpenSysCols() {
    char full_colname[255];
    strcpy(full_colname, sys_dbpath);
    strcat(full_colname, "/SYSCOLUMNS");
    RM_OpenFile(full_colname, col_file_handle);
}

RC CloseSysTables() {
    RM_CloseFile(table_file_handle);
}

RC CloseSysCols() {
    RM_CloseFile(col_file_handle);
}

RC CreateDB(char *dbpath, char *dbname) {
    char sys_table_name[255];
    char sys_col_name[255];

    strcpy(sys_table_name, dbpath);
    strcat(sys_table_name, "/SYSTABLES"); // todo: in windows may be different

    strcpy(sys_col_name, dbpath);
    strcat(sys_col_name, "/SYSCOLUMNS");   // todo: in windows may be different

    RM_CreateFile(sys_table_name, TABLE_ROW_SIZE + sizeof(bool) + sizeof(RID));
    RM_CreateFile(sys_col_name, COL_ROW_SIZE + sizeof(bool) + sizeof(RID));

    strcpy(sys_dbpath, dbpath);
    strcpy(sys_dbname, dbname);
    return SUCCESS;
}

RC DropDB(char *dbname) {
    char sys_table_name[255] = "rm ";   // todo: in windows may be different
    char sys_col_name[255] = "rm ";
    char sys_db_name[255] = "rm ";
    char sys_ix_name[255] = "rm ";

    strcat(sys_table_name, sys_dbpath);
    strcat(sys_table_name, "/SYSTABLES");

    strcat(sys_col_name, sys_dbpath);
    strcat(sys_col_name, "/SYSCOLUMNS");

    strcat(sys_db_name, sys_dbpath);
    strcat(sys_db_name, "/");
    strcat(sys_db_name, dbname);
    strcat(sys_db_name, ".tb*");

    strcat(sys_ix_name, sys_dbpath);
    strcat(sys_ix_name, "/");
    strcat(sys_ix_name, dbname);
    strcat(sys_ix_name, ".ix*");

    system(sys_table_name);
    system(sys_col_name);
    system(sys_db_name);
    system(sys_ix_name);

    return SUCCESS;
}

RC OpenDB(char *dbname) {
    table_file_handle = new RM_FileHandle;
    col_file_handle = new RM_FileHandle;
    strcpy(sys_dbname, dbname);
    return SUCCESS;
}

RC CloseDB() {
    delete table_file_handle;
    delete col_file_handle;
    memset(sys_dbname, 0, 255);
    return SUCCESS;
}

RC execute(char *sql) {
    return PF_NOBUF;
}

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes) {
    RID dump_rid;
    auto table_data = new char[TABLE_ROW_SIZE];
    strcpy(table_data, relName);
    memcpy(table_data + TABLENAME_SIZE, &attrCount, sizeof(int));
    OpenSysTables();
    InsertRec(table_file_handle, table_data, &dump_rid);
    CloseSysTables();
    delete[] table_data;

    auto attrOffset = 0;
    OpenSysCols();
    for (int i = 0; i < attrCount; i++) {
        auto curAttr = attributes[i];
        char curData[COL_ROW_SIZE];
        size_t offset = 0;
        strcpy(curData + offset, relName);
        offset += TABLENAME_SIZE;
        strcpy(curData + offset, curAttr.attrName);
        offset += ATTRNAME_SIZE;
        memcpy(curData + offset, &curAttr.attrType, sizeof(int));
        offset += ATTRTYPE_SIZE;
        memcpy(curData + offset, &curAttr.attrLength, sizeof(int));
        offset += ATTRLENGTH_SIZE;
        memcpy(curData + offset, &attrOffset, sizeof(int));
        // ix is empty here
        attrOffset += curAttr.attrLength;
        InsertRec(col_file_handle, curData, &dump_rid);
        printf("%s insert rid: %d %d\n", relName, dump_rid.pageNum, dump_rid.slotNum);
    }
    CloseSysCols();

    char full_tab_name[255];
    strcpy(full_tab_name, sys_dbname);
    strcat(full_tab_name, ".tb.");
    strcat(full_tab_name, relName);
    RM_CreateFile(full_tab_name, attrOffset + sizeof(RID) + sizeof(bool)); // which is datasize + rid.size
    return SUCCESS;
}

RC DropTable(char *relName) {
    Con table_condition;
    table_condition.bLhsIsAttr = 1;
    table_condition.bRhsIsAttr = 0;
    table_condition.compOp = EQual;
    table_condition.attrType = chars;
    table_condition.LattrOffset = 0;
    table_condition.LattrLength = 21;
    table_condition.Rvalue = relName;
    int col_num = 0;

    OpenSysTables();
    RM_Record table_record;
    RM_FileScan table_scan;
    OpenScan(&table_scan, table_file_handle, 1, &table_condition);
    RC is_existed = GetNextRec(&table_scan, &table_record);
    if (is_existed != SUCCESS) {
        return is_existed;
    }
    memcpy(&col_num, table_record.pData + TABLENAME_SIZE, sizeof(int));
    DeleteRec(table_file_handle, &table_record.rid);
    CloseScan(&table_scan);
    CloseSysTables();

    OpenSysCols();
    RM_FileScan col_scan;
    RM_Record col_record;
    OpenScan(&col_scan, col_file_handle, 1, &table_condition);
    RID removed_rids[col_num];
    int removed_num = 0;
    while (true) {
        is_existed = GetNextRec(&col_scan, &col_record);
        if (is_existed != SUCCESS) {
            break;
        }
        removed_rids[removed_num] = col_record.rid;
        removed_num++;
    }
    printf("removed num: %d\n", removed_num);
    for (int i = 0; i < removed_num; i++) {
        DeleteRec(col_file_handle, &removed_rids[i]);
    }
    CloseSysCols();

    char sys_db_name[255] = "rm ";
    char sys_ix_name[255] = "rm ";

    strcat(sys_db_name, sys_dbpath);
    strcat(sys_db_name, "/");
    strcat(sys_db_name, sys_dbname);
    strcat(sys_db_name, ".tb.");
    strcat(sys_db_name, relName);

    strcat(sys_ix_name, sys_dbpath);
    strcat(sys_ix_name, "/");
    strcat(sys_ix_name, sys_dbname);
    strcat(sys_ix_name, ".ix.");
    strcat(sys_ix_name, relName);
    strcat(sys_ix_name, "*");

    system(sys_db_name);
    system(sys_ix_name);
    return SUCCESS;
}

//RC CreateIndex(char *indexName, char *relName, char *attrName) {
//    for (int i = 0; i < sys_col_row_num; i++) {
//        char * curRow = sys_col_data + COL_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0 && strcmp(curRow + TABLENAME_SIZE, attrName) == 0) {
//            AttrType attrType;
//            int attrLength;
//            if (* (curRow + IX_FLAG_OFFSET) == (char) 1) {
//                return INDEX_EXIST;
//            }
//            memcpy(&attrType, curRow + TABLENAME_SIZE + ATTRNAME_SIZE, ATTRTYPE_SIZE);
//            memcpy(&attrLength, curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, ATTRLENGTH_SIZE);
//
//            char tmp = 1;
//            memcpy(curRow + IX_FLAG_OFFSET, &tmp, 1);
//            strcpy(curRow + IX_FLAG_OFFSET + 1, indexName);
//
//            char full_index_name[255];
//            strcpy(full_index_name, sys_dbname);
//            strcat(full_index_name, ".ix.");
//            strcat(full_index_name, relName);
//            strcat(full_index_name, ".");
//            strcat(full_index_name, indexName);
//
//            CreateIndex(full_index_name, attrType, attrLength);
//            return SUCCESS;
//        }
//    }
//    return TABLE_NOT_EXIST;
//}
//
//RC DropIndex(char *indexName) {
//    for (int i = 0; i < sys_col_row_num; i++) {
//        char * curRow = sys_col_data + COL_ROW_SIZE * i;
//        if (strcmp(curRow + IX_FLAG_OFFSET + 1, indexName) == 0) {
//            if (* (curRow + IX_FLAG_OFFSET) == (char) 0) {
//                return INDEX_NOT_EXIST;
//            }
//
//            char tmp = 0;
//            memcpy(curRow + IX_FLAG_OFFSET, &tmp, 1);
//            strcpy(curRow + IX_FLAG_OFFSET + 1, "");
//
//            char full_index_name[255] = "rm ";
//            strcat(full_index_name, sys_dbname);
//            strcat(full_index_name, ".ix.*.");
//            strcat(full_index_name, indexName);
//
//            system(full_index_name);
//            return SUCCESS;
//        }
//    }
//    return INDEX_NOT_EXIST;
//}
//
//RC Insert(char *relName, int nValues, Value *values) {
//    bool isFound = false;
//    int attr_len[nValues];
//    for (int i = 0; i < sys_table_row_num; i++) {
//        char * curRow = sys_table_data + TABLE_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0) {
//            isFound = true;
//            break;
//        }
//    }
//    if (!isFound) {
//        return TABLE_NOT_EXIST;
//    }
//
//    for (int i = 0; i < sys_col_row_num; i++) {
//        char * curRow = sys_col_data + COL_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0) {
//            for (int j = i; j < nValues; j++) {
//                int cur_len = 0;
//                AttrType cur_type;
//                curRow = sys_col_data + COL_ROW_SIZE * j;
//                memcpy(&cur_type, curRow + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
//                if (cur_type != values[j - i].type) { // to check whether the data is
//                    return RECORD_NOT_EXIST;
//                }
//                memcpy(&cur_len, curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
//                attr_len[j - i] = cur_len;
//            }
//            break;
//        }
//    }
//
//    char full_table_name[255];
//    strcpy(full_table_name, sys_dbname);
//    strcat(full_table_name, ".tb.");
//    strcat(full_table_name, relName);
//
//    char insert_value[MAX_FILE_SIZE];
//    int attr_offset = 0;
//    for (int i = 0; i < nValues; i++) {
//        char * curData = (char *) values[i].data;
//        memcpy(insert_value + attr_offset, curData, (size_t) attr_len[i]);
//        attr_offset += attr_len[i];
//    }
//
//    auto rm_handle = new RM_FileHandle;
//    RM_OpenFile(full_table_name, rm_handle);
//    auto insert_result = InsertRec(rm_handle, insert_value, &cur_rid);
//    RM_CloseFile(rm_handle);
//    delete rm_handle;
//    if (insert_result != SUCCESS) {
//        return insert_result;
//    }
//
//    for (int i = 0; i < sys_col_row_num; i++) {
//        char * curRow = sys_col_data + COL_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0) {
//            for (int j = i; j < nValues; j++) {
//                curRow = sys_col_data + COL_ROW_SIZE * j;
//                char is_inx = 0;
//                memcpy(&is_inx, curRow + IX_FLAG_OFFSET, 1);
//                if (is_inx == 1) {
//                    char ix_name[21];
//                    strcpy(ix_name, curRow + IX_FLAG_OFFSET + 1);
//
//                    char full_index_name[255];
//                    strcpy(full_index_name, sys_dbname);
//                    strcat(full_index_name, ".ix.");
//                    strcat(full_index_name, relName);
//                    strcat(full_index_name, ".");
//                    strcat(full_index_name, ix_name);
//
//                    auto ix_handle = new IX_IndexHandle;
//                    OpenIndex(full_index_name, ix_handle);
//                    InsertEntry(ix_handle, (char *) values[j - i].data, &cur_rid);
//                    CloseIndex(ix_handle);
//                    delete ix_handle;
//                }
//            }
//            break;
//        }
//    }
//    return SUCCESS;
//}
//
//RC Delete(char *relName, int nConditions, Condition *conditions) {
//    int col_num = 0;
//    RC table_exist = GetTableInfo(relName, &col_num);
//    if (table_exist == TABLE_NOT_EXIST) {
//        return TABLE_NOT_EXIST;
//    }
//
//    char *col_name[col_num];
//    int col_length[col_num];
//    int col_offset[col_num];
//    AttrType col_types[col_num];
//    bool col_is_indx[col_num];
//    char *col_indx_name[col_num];
//
//    for (int i = 0; i < col_num; i++) {
//        col_name[i] = new char[255];
//        col_indx_name[i] = new char[255];
//    }
//
//    GetColsInfo(relName, col_num, col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name);
//
//    auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);
//
//    char full_table_name[255];
//    strcpy(full_table_name, sys_dbname);
//    strcat(full_table_name, ".tb.");
//    strcat(full_table_name, relName);
//
//    auto rm_fileHandle = new RM_FileHandle;
//    auto rm_fileScan = new RM_FileScan;
//    RM_OpenFile(full_table_name, rm_fileHandle);
//    OpenScan(rm_fileScan, rm_fileHandle, nConditions, cons);
//    RID removed_rid[rm_fileHandle->rm_fileSubHeader->nRecords];
//    int removed_num = 0;
//
//    while (true) {
//        RM_Record record;
//        RC result = GetNextRec(rm_fileScan, &record);
//        if (result != SUCCESS)  {
//            break;
//        }
//
//        for (int i = 0; i < col_num; i++) {
//            if (col_is_indx[i] == 1) {
//                char full_index_name[255];
//                strcpy(full_index_name, sys_dbname);
//                strcat(full_index_name, ".ix.");
//                strcat(full_index_name, relName);
//                strcat(full_index_name, ".");
//                strcat(full_index_name, col_indx_name[i]);
//
//                auto cur_offset = col_offset[i];
//
//                auto index_handle = new IX_IndexHandle;
//                OpenIndex(full_index_name, index_handle);
//                DeleteEntry(index_handle, record.pData + cur_offset, &record.rid);
//                CloseIndex(index_handle);
//            }
//        }
//        removed_rid[removed_num] = record.rid;
//        removed_num++;
//    }
//    for (int i = 0; i < removed_num; i++) {
//        DeleteRec(rm_fileHandle, &removed_rid[i]);
//    }
//    CloseScan(rm_fileScan);
//    RM_CloseFile(rm_fileHandle);
//
//    for (int i = 0; i < col_num; i++) {
//        delete col_name[i];
//        delete col_indx_name[i];
//    }
//
//    return SUCCESS;
//}
//
//RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
//    int col_num = 0;
//    RC table_exist = GetTableInfo(relName, &col_num);
//    if (table_exist == TABLE_NOT_EXIST) {
//        return TABLE_NOT_EXIST;
//    }
//
//    char *col_name[col_num];
//    int col_length[col_num];
//    int col_offset[col_num];
//    AttrType col_types[col_num];
//    bool col_is_indx[col_num];
//    char *col_indx_name[col_num];
//
//    for (int i = 0; i < col_num; i++) {
//        col_name[i] = new char[255];
//        col_indx_name[i] = new char[255];
//    }
//
//    GetColsInfo(relName, col_num, col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name);
//
//    auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);
//
//    char full_table_name[255];
//    strcpy(full_table_name, sys_dbname);
//    strcat(full_table_name, ".tb.");
//    strcat(full_table_name, relName);
//
//    auto rm_fileHandle = new RM_FileHandle;
//    auto rm_fileScan = new RM_FileScan;
//    RM_OpenFile(full_table_name, rm_fileHandle);
//    OpenScan(rm_fileScan, rm_fileHandle, nConditions, cons);
//
//    while (true) {
//        RM_Record record;
//        RC result = GetNextRec(rm_fileScan, &record);
//        if (result != SUCCESS)  {
//            break;
//        }
//
//        auto data_size = rm_fileHandle->rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID);
//        char updated_data[data_size];
//        memcpy(updated_data, record.pData, sizeof(data_size));
//        for (int i = 0; i < col_num; i++) {
//            if (strcmp(col_name[i], attrName) == 0) {
//                memcpy(updated_data + col_offset[i], value->data, (size_t) col_length[i]);
//
//                if (col_is_indx[i] == 1) {
//                    char full_index_name[255];
//                    strcpy(full_index_name, sys_dbname);
//                    strcat(full_index_name, ".ix.");
//                    strcat(full_index_name, relName);
//                    strcat(full_index_name, ".");
//                    strcat(full_index_name, col_indx_name[i]);
//
//                    auto cur_offset = col_offset[i];
//
//                    auto index_handle = new IX_IndexHandle;
//                    OpenIndex(full_index_name, index_handle);
//                    DeleteEntry(index_handle, record.pData + cur_offset, &record.rid);
//                    InsertEntry(index_handle, (char *) value->data, &record.rid);
//                    CloseIndex(index_handle);
//                }
//                break;
//            }
//        }
//        record.pData = updated_data;
//        UpdateRec(rm_fileHandle, &record);
//    }
//    CloseScan(rm_fileScan);
//    RM_CloseFile(rm_fileHandle);
//
//    for (int i = 0; i < col_num; i++) {
//        delete col_name[i];
//        delete col_indx_name[i];
//    }
//
//    return PF_NOBUF;
//}
//
//RC GetColsInfo(char *relName, int colNum, char ** attrName, AttrType * attrType, int * attrLength, int * attrOffset,
//               bool * ixFlag, char ** indexName) {
//
//    for (int i = 0; i < sys_col_row_num; i++) {
//        char * curRow = sys_col_data + COL_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0) {
//            for (int j = i; j < colNum; j++) {
//                curRow = sys_col_data + COL_ROW_SIZE * j;
//                int cur_pos = j - i;
//                strcpy(attrName[cur_pos], curRow + TABLENAME_SIZE);
//                memcpy(&attrType[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
//                memcpy(&attrLength[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
//                memcpy(&attrOffset[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE + ATTRLENGTH_SIZE,
//                       sizeof(int));
//                memcpy(&ixFlag[cur_pos], curRow + IX_FLAG_OFFSET, 1);
//                if (ixFlag[cur_pos] == 1) {
//                    strcpy(indexName[cur_pos], curRow + IX_FLAG_OFFSET + 1);
//                }
//            }
//            break;
//        }
//    }
//    return SUCCESS;
//}
//
//RC GetTableInfo(char *relName, int *colNum) {
//    bool isFound = false;
//    for (int i = 0; i < sys_table_row_num; i++) {
//        char * curRow = sys_table_data + TABLE_ROW_SIZE * i;
//        if (strcmp(curRow, relName) == 0) {
//            memcpy(colNum, curRow + TABLENAME_SIZE, sizeof(int));
//            isFound = true;
//            break;
//        }
//    }
//    if (!isFound) {
//        return TABLE_NOT_EXIST;
//    }
//    return SUCCESS;
//}