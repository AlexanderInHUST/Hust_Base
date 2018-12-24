//
// Created by 唐艺峰 on 2018/12/18.
//

#include "SYS_Manager.h"
#include "Condition_tool.h"

char sys_dbpath[255];
char sys_dbname[255];
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
    int col_num;
    Con table_condition;
    table_condition.bLhsIsAttr = 1;
    table_condition.bRhsIsAttr = 0;
    table_condition.compOp = EQual;
    table_condition.attrType = chars;
    table_condition.LattrOffset = 0;
    table_condition.LattrLength = 21;
    table_condition.Rvalue = relName;

    OpenSysTables();
    RM_Record table_record;
    RM_FileScan table_scan;
    OpenScan(&table_scan, table_file_handle, 1, &table_condition);
    RC is_existed = GetNextRec(&table_scan, &table_record);
    if (is_existed == SUCCESS) {
        DEBUG_LOG("Return Code: %d Table \"%s\" exists\n", is_existed, relName);

        CloseScan(&table_scan);
        CloseSysTables();
        return TABLE_EXIST;
    }
    memcpy(&col_num, table_record.pData + TABLENAME_SIZE, sizeof(int));
    CloseScan(&table_scan);
    CloseSysTables();

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
    }
    CloseSysCols();

    char full_tab_name[255];
    strcpy(full_tab_name, sys_dbpath);
    strcat(full_tab_name, "/");
    strcat(full_tab_name, sys_dbname);
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
        DEBUG_LOG("Return Code: %d Table \"%s\" doesn't exist\n", is_existed, relName);

        CloseScan(&table_scan);
        CloseSysTables();
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
    ALL_LOG("%d records has been removed\n", removed_num);
    for (int i = 0; i < removed_num; i++) {
        DeleteRec(col_file_handle, &removed_rids[i]);
    }
    CloseScan(&col_scan);
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

RC CreateIndex(char *indexName, char *relName, char *attrName) {
    Con cols_condition[2];
    cols_condition[0].bLhsIsAttr = 1;
    cols_condition[0].bRhsIsAttr = 0;
    cols_condition[0].compOp = EQual;
    cols_condition[0].attrType = chars;
    cols_condition[0].LattrOffset = 0;
    cols_condition[0].LattrLength = 21;
    cols_condition[0].Rvalue = relName;

    cols_condition[1].bLhsIsAttr = 1;
    cols_condition[1].bRhsIsAttr = 0;
    cols_condition[1].compOp = EQual;
    cols_condition[1].attrType = chars;
    cols_condition[1].LattrOffset = 21;
    cols_condition[1].LattrLength = 21;
    cols_condition[1].Rvalue = attrName;

    OpenSysCols();
    RM_Record col_record;
    RM_FileScan col_scan;
    OpenScan(&col_scan, col_file_handle, 2, cols_condition);
    RC is_existed = GetNextRec(&col_scan, &col_record);
    if (is_existed != SUCCESS) {
        DEBUG_LOG("Return Code: %d Attr \"%s.%s\" doesn't exist\n", is_existed, relName, attrName);
        CloseScan(&col_scan);
        CloseSysCols();
        return is_existed;
    }
    CloseScan(&col_scan); // check whether attr exists

    cols_condition[0].bLhsIsAttr = 1;
    cols_condition[0].bRhsIsAttr = 0;
    cols_condition[0].compOp = EQual;
    cols_condition[0].attrType = chars;
    cols_condition[0].LattrOffset = IX_FLAG_OFFSET + 1;
    cols_condition[0].LattrLength = 21;
    cols_condition[0].Rvalue = indexName;

    OpenScan(&col_scan, col_file_handle, 1, cols_condition);
    is_existed = GetNextRec(&col_scan, &col_record);
    if (is_existed == SUCCESS) {
        DEBUG_LOG("Return Code: %d Index \"%s\" has been used\n", is_existed, indexName);
        CloseScan(&col_scan);
        CloseSysCols();
        return is_existed;
    }
    CloseScan(&col_scan); // check whether index name has been used

    auto cur_row = col_record.pData;
    AttrType attrType;
    int attrLength;
    char tmp = 1;

    if (* (cur_row + IX_FLAG_OFFSET) == (char) 1) {
        DEBUG_LOG("Return Code: %d Attr \"%s.%s\" has been an index\n", INDEX_EXIST, relName, attrName);
        CloseSysCols();
        return INDEX_EXIST;
    }
    memcpy(&attrType, cur_row + TABLENAME_SIZE + ATTRNAME_SIZE, ATTRTYPE_SIZE);
    memcpy(&attrLength, cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, ATTRLENGTH_SIZE);
    memcpy(cur_row + IX_FLAG_OFFSET, &tmp, 1);
    strcpy(cur_row + IX_FLAG_OFFSET + 1, indexName);
    UpdateRec(col_file_handle, &col_record);
    CloseSysCols();

    char full_index_name[255];
    strcpy(full_index_name, sys_dbpath);
    strcat(full_index_name, "/");
    strcat(full_index_name, sys_dbname);
    strcat(full_index_name, ".ix.");
    strcat(full_index_name, relName);
    strcat(full_index_name, ".");
    strcat(full_index_name, indexName);

    RC create_result = CreateIndex(full_index_name, attrType, attrLength);
    if (create_result != SUCCESS) {
        DEBUG_LOG("Return Code: %d Attr \"%s.%s\" fails to create index with exception\n", create_result, relName, attrName);
    }
    return create_result;
}


RC DropIndex(char *indexName) {
    Con cols_condition;
    cols_condition.bLhsIsAttr = 1;
    cols_condition.bRhsIsAttr = 0;
    cols_condition.compOp = EQual;
    cols_condition.attrType = chars;
    cols_condition.LattrOffset = IX_FLAG_OFFSET + 1;
    cols_condition.LattrLength = 21;
    cols_condition.Rvalue = indexName;

    RM_Record col_record;
    RM_FileScan col_scan;
    OpenSysCols();
    OpenScan(&col_scan, col_file_handle, 1, &cols_condition);
    RC is_existed = GetNextRec(&col_scan, &col_record);
    if (is_existed != SUCCESS) {
        DEBUG_LOG("Return Code: %d Index \"%s\" dosen't exist\n", is_existed, indexName);
        CloseScan(&col_scan);
        CloseSysCols();
        return is_existed;
    }
    CloseScan(&col_scan); // check whether index name has been used

    int tmp = 0;
    auto cur_row = col_record.pData;
    char relName[21];
    strcpy(relName, cur_row);
    memcpy(cur_row + IX_FLAG_OFFSET, &tmp, 1);
    strcpy(cur_row + IX_FLAG_OFFSET + 1, "");
    UpdateRec(col_file_handle, &col_record);
    CloseSysCols();

    char full_index_name[255] = "rm ";
    strcat(full_index_name, sys_dbpath);
    strcat(full_index_name, "/");
    strcat(full_index_name, sys_dbname);
    strcat(full_index_name, ".ix.");
    strcat(full_index_name, relName);
    strcat(full_index_name, ".");
    strcat(full_index_name, indexName);

    system(full_index_name);
    return SUCCESS;
}

RC Insert(char *relName, int nValues, Value *values) {
    RID cur_rid;
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
        DEBUG_LOG("Return Code: %d Table \"%s\" doesn't exist\n", is_existed, relName);

        CloseScan(&table_scan);
        CloseSysTables();
        return is_existed;
    }
    memcpy(&col_num, table_record.pData + TABLENAME_SIZE, sizeof(int));
    CloseScan(&table_scan);
    CloseSysTables();

    int attr_len[col_num], attr_pos = 0;
    bool is_index[col_num];
    char index_name[col_num][255];
    OpenSysCols();
    OpenScan(&table_scan, col_file_handle, 1, &table_condition);
    while (true) {
        AttrType attr_type;
        is_existed = GetNextRec(&table_scan, &table_record);
        if (is_existed != SUCCESS) {
            break;
        }
        auto cur_row = table_record.pData;
        memcpy(&attr_type, cur_row + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
        if (attr_type != values[attr_pos].type) {
            DEBUG_LOG("Return Code: %d Table \"%s\" cannot contain this value caused by %d-th value type\n",
                    RM_INVALIDRECSIZE, relName, attr_pos);

            CloseScan(&table_scan);
            CloseSysCols();
            return RM_INVALIDRECSIZE;
        }
        memcpy(&attr_len[attr_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
        memcpy(&is_index[attr_pos], cur_row + IX_FLAG_OFFSET, 1);
        strcpy(index_name[attr_pos], cur_row + IX_FLAG_OFFSET + 1);
        attr_pos++;
    }
    CloseScan(&table_scan);
    CloseSysCols();

    char full_tab_name[255];
    strcpy(full_tab_name, sys_dbpath);
    strcat(full_tab_name, "/");
    strcat(full_tab_name, sys_dbname);
    strcat(full_tab_name, ".tb.");
    strcat(full_tab_name, relName);

    char insert_value[MAX_FILE_SIZE];
    int attr_offset = 0;
    for (int i = 0; i < nValues; i++) {
        char * cur_data = (char *) values[i].data;
        memcpy(insert_value + attr_offset, cur_data, (size_t) attr_len[i]);
        attr_offset += attr_len[i];
    }

    auto rm_handle = new RM_FileHandle;
    RM_OpenFile(full_tab_name, rm_handle);
    auto insert_result = InsertRec(rm_handle, insert_value, &cur_rid);
    RM_CloseFile(rm_handle);
    delete rm_handle;
    if (insert_result != SUCCESS) {
        DEBUG_LOG("Return Code: %d Table \"%s\" inserting record fails\n",
                  insert_result, relName);
        return insert_result;
    }   //  insert into table

    for (int i = 0; i < col_num; i++) {
        if (is_index[i] == 1) {
            char full_index_name[255];
            strcpy(full_index_name, sys_dbpath);
            strcat(full_index_name, "/");
            strcat(full_index_name, sys_dbname);
            strcat(full_index_name, ".ix.");
            strcat(full_index_name, relName);
            strcat(full_index_name, ".");
            strcat(full_index_name, index_name[i]);

            auto index_handle = new IX_IndexHandle;
            OpenIndex(full_index_name, index_handle);
            insert_result = InsertEntry(index_handle, (char *) values[i].data, &cur_rid);
            if (insert_result != SUCCESS) {
                DEBUG_LOG("Return Code: %d Index \"%s\" inserting idx fails\n",
                          insert_result, index_name[i]);
            }
            printf("cur_rid : %d.%d, insert into %s: %s\n", cur_rid.pageNum,cur_rid.slotNum, index_name[i], (char *) values[i].data);
            CloseIndex(index_handle);
            delete index_handle;
        }
    }
    return SUCCESS;
}

RC Delete(char *relName, int nConditions, Condition *conditions) {
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
        DEBUG_LOG("Return Code: %d Table \"%s\" doesn't exist\n", is_existed, relName);

        CloseScan(&table_scan);
        CloseSysTables();
        return is_existed;
    }
    memcpy(&col_num, table_record.pData + TABLENAME_SIZE, sizeof(int));
    CloseScan(&table_scan);
    CloseSysTables();

    char *col_name[col_num];
    int col_length[col_num];
    int col_offset[col_num];
    AttrType col_types[col_num];
    bool col_is_indx[col_num];
    char *col_indx_name[col_num];

    for (int i = 0; i < col_num; i++) {
        col_name[i] = new char[255];
        col_indx_name[i] = new char[255];
    }

    GetColsInfo(relName, col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name);
    auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);

    char full_tab_name[255];
    strcpy(full_tab_name, sys_dbpath);
    strcat(full_tab_name, "/");
    strcat(full_tab_name, sys_dbname);
    strcat(full_tab_name, ".tb.");
    strcat(full_tab_name, relName);

    RM_FileHandle rm_fileHandle;
    RM_FileScan rm_fileScan;
    RM_OpenFile(full_tab_name, &rm_fileHandle);
    OpenScan(&rm_fileScan, &rm_fileHandle, nConditions, cons);
    RID removed_rid[rm_fileHandle.rm_fileSubHeader->nRecords];
    char removed_data[rm_fileHandle.rm_fileSubHeader->nRecords][rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
    int removed_num = 0;

    while (true) {
        RM_Record record;
        RC result = GetNextRec(&rm_fileScan, &record);
        if (result != SUCCESS)  {
            break;
        }
        removed_rid[removed_num] = record.rid;
        memcpy(removed_data[removed_num], record.pData, (size_t) rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID));
        removed_num++;
    }
    CloseScan(&rm_fileScan);
    RM_CloseFile(&rm_fileHandle);

    printf("%d records in %s will be deleted.\n", removed_num, relName);

    for (int i = 0; i < col_num; i++) {
        if (col_is_indx[i]) {
            char full_index_name[255];
            strcpy(full_index_name, sys_dbpath);
            strcat(full_index_name, "/");
            strcat(full_index_name, sys_dbname);
            strcat(full_index_name, ".ix.");
            strcat(full_index_name, relName);
            strcat(full_index_name, ".");
            strcat(full_index_name, col_indx_name[i]);

            IX_IndexHandle ix_indexHandle;
            OpenIndex(full_index_name, &ix_indexHandle);
            for (int j = 0; j < removed_num; j++) {
                DeleteEntry(&ix_indexHandle, removed_data[j] + col_offset[i], &removed_rid[j]);
            }
            CloseIndex(&ix_indexHandle);
        }
    }

    for (int i = 0; i < col_num; i++) {
        delete col_name[i];
        delete col_indx_name[i];
    }

    return SUCCESS;
}
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
RC GetColsInfo(char *relName, char ** attrName, AttrType * attrType, int * attrLength, int * attrOffset,
               bool * ixFlag, char ** indexName) {
    Con table_condition;
    table_condition.bLhsIsAttr = 1;
    table_condition.bRhsIsAttr = 0;
    table_condition.compOp = EQual;
    table_condition.attrType = chars;
    table_condition.LattrOffset = 0;
    table_condition.LattrLength = 21;
    table_condition.Rvalue = relName;
    int cur_pos = 0;

    OpenSysCols();
    RM_FileScan col_scan;
    RM_Record col_record;
    OpenScan(&col_scan, col_file_handle, 1, &table_condition);
    while (true) {
        RC is_existed = GetNextRec(&col_scan, &col_record);
        if (is_existed != SUCCESS) {
            break;
        }
        auto cur_row = col_record.pData;
        strcpy(attrName[cur_pos], cur_row + TABLENAME_SIZE);
        memcpy(&attrType[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
        memcpy(&attrLength[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
        memcpy(&attrOffset[cur_pos], cur_row + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE + ATTRLENGTH_SIZE,
               sizeof(int));
        memcpy(&ixFlag[cur_pos], cur_row + IX_FLAG_OFFSET, 1);
        if (ixFlag[cur_pos] == 1) {
            strcpy(indexName[cur_pos], cur_row + IX_FLAG_OFFSET + 1);
        }
        cur_pos++;
    }
    CloseScan(&col_scan);
    CloseSysCols();
    return SUCCESS;
}

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