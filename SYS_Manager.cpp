//
// Created by 唐艺峰 on 2018/12/18.
//

#include "SYS_Manager.h"

char sys_dbname[255];
int sys_table_fd = -1;
int sys_col_fd = -1;
int sys_table_row_num = 0;
int sys_col_row_num = 0;
char * sys_table_data = nullptr;
char * sys_col_data = nullptr;

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

    sys_table_fd = open(full_dbname, O_RDWR | O_CREAT | O_EXCL, //  | O_BINARY in windows
                        S_IREAD | S_IWRITE);
    sys_col_fd = open(full_colname, O_RDWR | O_CREAT | O_EXCL, //  | O_BINARY in windows
                      S_IREAD | S_IWRITE);
    if (sys_table_fd < 0 || sys_col_fd < 0) {
        return TABLE_NOT_EXIST;
    }

    read(sys_table_fd, &sys_table_row_num, sizeof(int));
    read(sys_table_fd, &sys_table_data, TABLE_ROW_SIZE * sys_table_row_num);
    read(sys_col_fd, &sys_col_row_num, sizeof(int));
    read(sys_col_fd, &sys_col_data, COL_ROW_SIZE * sys_col_row_num);
    return SUCCESS;
}

RC CloseDB() {
    if (sys_table_fd == -1 || sys_col_fd == -1) {
        return TABLE_NOT_EXIST;
    }


    return PF_NOBUF;
}

RC execute(char *sql) {
    return PF_NOBUF;
}

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes) {
    return PF_NOBUF;
}

RC DropTable(char *relName) {
    return PF_NOBUF;
}

RC CreateIndex(char *indexName, char *relName, char *attrName) {
    return PF_NOBUF;
}

RC DropIndex(char *indexName) {
    return PF_NOBUF;
}

RC Insert(char *relName, int nValues, Value *values) {
    return PF_NOBUF;
}

RC Delete(char *relName, int nConditions, Condition *conditions) {
    return PF_NOBUF;
}

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
    return PF_NOBUF;
}
