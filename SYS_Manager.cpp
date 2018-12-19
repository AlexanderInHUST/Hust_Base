//
// Created by 唐艺峰 on 2018/12/18.
//

#include "SYS_Manager.h"
#include "Condition_tool.h"

char sys_dbname[255];
int sys_table_fd = -1;
int sys_col_fd = -1;
int sys_table_row_num = 0;
int sys_col_row_num = 0;
char * sys_table_data = nullptr;
char * sys_col_data = nullptr;
RID cur_rid;

RC CreateDB(char *dbpath, char *dbname) {
    char sys_table_name[255];
    char sys_col_name[255];

    strcpy(sys_table_name, dbpath);
    strcat(sys_table_name, "/"); // todo: in windows may be different
    strcat(sys_table_name, dbname);
    strcat(sys_table_name, ".db");

    strcpy(sys_col_name, dbpath);
    strcat(sys_col_name, "/");   // todo: in windows may be different
    strcat(sys_col_name, dbname);
    strcat(sys_col_name, ".col");

    sys_table_fd = open(sys_table_name, O_RDWR | O_CREAT | O_EXCL, //  | O_BINARY in windows
              S_IREAD | S_IWRITE);
    if (sys_table_fd < 0) {
        return DB_EXIST;
    }

    sys_col_fd = open(sys_col_name, O_RDWR | O_CREAT | O_EXCL, //  | O_BINARY in windows
                        S_IREAD | S_IWRITE);
    if (sys_col_fd < 0) {
        return DB_EXIST;
    }
    return SUCCESS;
}

RC DropDB(char *dbname) {
    char sys_table_name[255] = "rm ";   // todo: in windows may be different
    char sys_col_name[255] = "rm ";
    char sys_db_name[255] = "rm ";
    char sys_ix_name[255] = "rm ";

    strcat(sys_table_name, dbname);
    strcat(sys_table_name, ".db");
    strcat(sys_col_name, dbname);
    strcat(sys_col_name, ".col");
    strcat(sys_db_name, dbname);
    strcat(sys_db_name, ".tb*");
    strcat(sys_ix_name, dbname);
    strcat(sys_ix_name, ".ix*");

    system(sys_table_name);
    system(sys_col_name);
    system(sys_db_name);
    system(sys_ix_name);

    return SUCCESS;
}

RC OpenDB(char *dbname) {
    strcpy(sys_dbname, dbname);
    char full_dbname[255];
    char full_colname[255];

    strcpy(full_dbname, dbname);
    strcat(full_dbname, ".db");

    strcpy(full_colname, dbname);
    strcat(full_colname, ".col");

    sys_table_fd = open(full_dbname, O_RDWR);
    sys_col_fd = open(full_colname, O_RDWR);
    if (sys_table_fd < 0 || sys_col_fd < 0) {
        return TABLE_NOT_EXIST;
    }

    read(sys_table_fd, &sys_table_row_num, sizeof(int));
    sys_table_data = new char[MAX_FILE_SIZE];
    read(sys_table_fd, sys_table_data, TABLE_ROW_SIZE * sys_table_row_num);

    read(sys_col_fd, &sys_col_row_num, sizeof(int));
    sys_col_data = new char[MAX_FILE_SIZE];
    read(sys_col_fd, sys_col_data, COL_ROW_SIZE * sys_col_row_num);

    return SUCCESS;
}

RC CloseDB() {
    if (sys_table_fd == -1 || sys_col_fd == -1) {
        return TABLE_NOT_EXIST;
    }
    lseek(sys_table_fd, 0, 0);
    write(sys_table_fd, &sys_table_row_num, sizeof(int));
    write(sys_table_fd, sys_table_data, sys_table_row_num * TABLE_ROW_SIZE);
    close(sys_table_fd);
    delete sys_table_data;

    lseek(sys_col_fd, 0, 0);
    write(sys_col_fd, &sys_col_row_num, sizeof(int));
    write(sys_col_fd, sys_col_data, sys_col_row_num * COL_ROW_SIZE);
    close(sys_col_fd);
    delete sys_col_data;

    return SUCCESS;
}

RC execute(char *sql) {
    return PF_NOBUF;
}

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes) {
    memset(sys_table_data + sys_table_row_num * TABLE_ROW_SIZE, 0, sizeof(TABLE_ROW_SIZE));
    strcpy(sys_table_data + sys_table_row_num * TABLE_ROW_SIZE, relName);
    memcpy(sys_table_data + sys_table_row_num * TABLE_ROW_SIZE + TABLENAME_SIZE, &attrCount, sizeof(int));
    sys_table_row_num++;

    memset(sys_col_data + sys_col_row_num * COL_ROW_SIZE, 0, sizeof(COL_ROW_SIZE) * attrCount);
    auto attrOffset = 0;
    for (int i = 0; i < attrCount; i++) {
        auto curAttr = attributes[i];
        auto curData = sys_col_data + (sys_col_row_num + i) * COL_ROW_SIZE;
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
    }
    sys_col_row_num += attrCount;

    char full_tab_name[255];
    strcpy(full_tab_name, sys_dbname);
    strcat(full_tab_name, ".tb.");
    strcat(full_tab_name, relName);
    RM_CreateFile(full_tab_name, attrOffset + sizeof(RID) + sizeof(bool)); // which is datasize + rid.size
    return SUCCESS;
}

RC DropTable(char *relName) {
    bool isFound = false;
    int attrCount = 0;
    for (int i = 0; i < sys_table_row_num; i++) {
        char * curRow = sys_table_data + TABLE_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            memcpy(&attrCount, curRow + TABLENAME_SIZE, sizeof(int));
            isFound = true;
            for (int j = i + 1; j < sys_table_row_num; j++) {
                curRow = sys_table_data + TABLE_ROW_SIZE * j;
                memcpy(curRow - TABLE_ROW_SIZE, curRow, TABLE_ROW_SIZE);
            }
            sys_table_row_num--;
            break;
        }
    }
    if (!isFound) {
        return TABLE_NOT_EXIST;
    }

    isFound = false;
    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            isFound = true;
            for (int j = i + attrCount; j < sys_col_row_num; j++) {
                curRow = sys_col_data + COL_ROW_SIZE * j;
                memcpy(curRow - attrCount * COL_ROW_SIZE, curRow, COL_ROW_SIZE);
            }
            sys_col_row_num -= attrCount;
            break;
        }
    }
    if (!isFound) {
        return TABLE_NOT_EXIST;
    }

    char sys_db_name[255] = "rm ";
    char sys_ix_name[255] = "rm ";
    strcat(sys_db_name, sys_dbname);
    strcat(sys_db_name, ".tb.");
    strcat(sys_db_name, relName);
    strcat(sys_ix_name, sys_dbname);
    strcat(sys_ix_name, ".ix.");
    strcat(sys_ix_name, relName);
    strcat(sys_ix_name, "*");
    system(sys_db_name);
    system(sys_ix_name);
    return SUCCESS;
}

RC CreateIndex(char *indexName, char *relName, char *attrName) {
    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0 && strcmp(curRow + TABLENAME_SIZE, attrName) == 0) {
            AttrType attrType;
            int attrLength;
            if (* (curRow + IX_FLAG_OFFSET) == (char) 1) {
                return INDEX_EXIST;
            }
            memcpy(&attrType, curRow + TABLENAME_SIZE + ATTRNAME_SIZE, ATTRTYPE_SIZE);
            memcpy(&attrLength, curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, ATTRLENGTH_SIZE);

            char tmp = 1;
            memcpy(curRow + IX_FLAG_OFFSET, &tmp, 1);
            strcpy(curRow + IX_FLAG_OFFSET + 1, indexName);

            char full_index_name[255];
            strcpy(full_index_name, sys_dbname);
            strcat(full_index_name, ".ix.");
            strcat(full_index_name, relName);
            strcat(full_index_name, ".");
            strcat(full_index_name, indexName);

            CreateIndex(full_index_name, attrType, attrLength);
            return SUCCESS;
        }
    }
    return TABLE_NOT_EXIST;
}

RC DropIndex(char *indexName) {
    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow + IX_FLAG_OFFSET + 1, indexName) == 0) {
            if (* (curRow + IX_FLAG_OFFSET) == (char) 0) {
                return INDEX_NOT_EXIST;
            }

            char tmp = 0;
            memcpy(curRow + IX_FLAG_OFFSET, &tmp, 1);
            strcpy(curRow + IX_FLAG_OFFSET + 1, "");

            char full_index_name[255] = "rm ";
            strcat(full_index_name, sys_dbname);
            strcat(full_index_name, ".ix.*.");
            strcat(full_index_name, indexName);

            system(full_index_name);
            return SUCCESS;
        }
    }
    return INDEX_NOT_EXIST;
}

RC Insert(char *relName, int nValues, Value *values) {
    bool isFound = false;
    int attr_len[nValues];
    for (int i = 0; i < sys_table_row_num; i++) {
        char * curRow = sys_table_data + TABLE_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            isFound = true;
            break;
        }
    }
    if (!isFound) {
        return TABLE_NOT_EXIST;
    }

    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            for (int j = i; j < nValues; j++) {
                int cur_len = 0;
                AttrType cur_type;
                curRow = sys_col_data + COL_ROW_SIZE * j;
                memcpy(&cur_type, curRow + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
                if (cur_type != values[j - i].type) { // to check whether the data is
                    return RECORD_NOT_EXIST;
                }
                memcpy(&cur_len, curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
                attr_len[j - i] = cur_len;
            }
            break;
        }
    }

    char full_table_name[255];
    strcpy(full_table_name, sys_dbname);
    strcat(full_table_name, ".tb.");
    strcat(full_table_name, relName);

    char insert_value[MAX_FILE_SIZE];
    int attr_offset = 0;
    for (int i = 0; i < nValues; i++) {
        char * curData = (char *) values[i].data;
        memcpy(insert_value + attr_offset, curData, (size_t) attr_len[i]);
        attr_offset += attr_len[i];
    }

    auto rm_handle = new RM_FileHandle;
    RM_OpenFile(full_table_name, rm_handle);
    auto insert_result = InsertRec(rm_handle, insert_value, &cur_rid);
    RM_CloseFile(rm_handle);
    delete rm_handle;
    if (insert_result != SUCCESS) {
        return insert_result;
    }

    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            for (int j = i; j < nValues; j++) {
                curRow = sys_col_data + COL_ROW_SIZE * j;
                char is_inx = 0;
                memcpy(&is_inx, curRow + IX_FLAG_OFFSET, 1);
                if (is_inx == 1) {
                    char ix_name[21];
                    strcpy(ix_name, curRow + IX_FLAG_OFFSET + 1);

                    char full_index_name[255];
                    strcpy(full_index_name, sys_dbname);
                    strcat(full_index_name, ".ix.");
                    strcat(full_index_name, relName);
                    strcat(full_index_name, ".");
                    strcat(full_index_name, ix_name);

                    auto ix_handle = new IX_IndexHandle;
                    OpenIndex(full_index_name, ix_handle);
                    InsertEntry(ix_handle, (char *) values[j - i].data, &cur_rid);
                    CloseIndex(ix_handle);
                    delete ix_handle;
                }
            }
            break;
        }
    }
    return SUCCESS;
}

RC Delete(char *relName, int nConditions, Condition *conditions) {
    bool isFound = false;
    int col_num = 0;
    for (int i = 0; i < sys_table_row_num; i++) {
        char * curRow = sys_table_data + TABLE_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            memcpy(&col_num, curRow + TABLENAME_SIZE, sizeof(int));
            isFound = true;
            break;
        }
    }
    if (!isFound) {
        return TABLE_NOT_EXIST;
    }

    char col_name[col_num][255];
    int col_length[col_num];
    int col_offset[col_num];
    AttrType col_types[col_num];

    for (int i = 0; i < sys_col_row_num; i++) {
        char * curRow = sys_col_data + COL_ROW_SIZE * i;
        if (strcmp(curRow, relName) == 0) {
            for (int j = i; j < col_num; j++) {
                curRow = sys_col_data + COL_ROW_SIZE * j;
                int cur_pos = j - i;
                strcpy(col_name[cur_pos], curRow + TABLENAME_SIZE);
                memcpy(&col_types[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE, sizeof(int));
                memcpy(&col_length[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
                memcpy(&col_offset[cur_pos], curRow + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE + ATTRLENGTH_SIZE,
                       sizeof(int));
            }
            break;
        }
    }

    auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);

    char full_table_name[255];
    strcpy(full_table_name, sys_dbname);
    strcat(full_table_name, ".tb.");
    strcat(full_table_name, relName);

    auto rm_fileHandle = new RM_FileHandle;
    auto rm_fileScan = new RM_FileScan;
    RM_OpenFile(full_table_name, rm_fileHandle);
    OpenScan(rm_fileScan, rm_fileHandle, nConditions, cons);
    RID removed_rid[rm_fileHandle->rm_fileSubHeader->nRecords];
    int removed_num = 0;

    while (true) {
        RM_Record record;
        RC result = GetNextRec(rm_fileScan, &record);
        if (result != SUCCESS)  {
            break;
        }
        removed_rid[removed_num] = record.rid;
        removed_num++;
    }

    for (int i = 0; i < removed_num; i++) {
        DeleteRec(rm_fileHandle, &removed_rid[i]);
    }

    CloseScan(rm_fileScan);
    RM_CloseFile(rm_fileHandle);
    return SUCCESS;
}

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
    return PF_NOBUF;
}
