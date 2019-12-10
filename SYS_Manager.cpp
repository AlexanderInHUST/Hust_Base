#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <iostream>
#include "Condition_tool.h"

char sys_dbpath[255];
char sys_dbname[255];
RM_FileHandle * table_file_handle;
RM_FileHandle * col_file_handle;
char sql_scripts[1024][255];

void ExecuteAndMessage(char * sql,CEditArea* editArea){//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;
	RC rc;
	if (s_sql.find("file") == 0) {
		CFileDialog createDialog(TRUE, NULL, _T("Script.sql"), 0, NULL, NULL);
		createDialog.m_ofn.lpstrTitle = "Load This Script";

		if (createDialog.DoModal() == IDOK) {
			CString allpath = createDialog.GetPathName();
			char *filename_cary = allpath.GetBuffer();
			FILE * script_file = fopen(filename_cary, "r");
			int scripts_num = 0;
			while (!feof(script_file)) {
				fgets(sql_scripts[scripts_num], 255, script_file);
				scripts_num++;
			}
			fclose(script_file);
			allpath.ReleaseBuffer();
			for (int i = 0; i < scripts_num; i++) {
				if (sql_scripts[i][0] == '\n' || sql_scripts[i][0] == '-') {
					continue;
				}
				AfxMessageBox(sql_scripts[i]);
				ExecuteAndMessage(sql_scripts[i], editArea);
			}
		}
		else {
			AfxMessageBox("Invalid path");
		}
		auto messages = new char*[1];
		messages[0] = "Success";
		editArea->ShowMessage(1, messages);
		delete[] messages;
		return;
	}
	else if(s_sql.find("select") == 0){
		SelResult res;
		Init_Result(&res);
		rc = Query(sql,&res);
		if (rc == SUCCESS) {
			int col_num = res.col_num;
			int row_num = 0;
			char ** fields = new char *[col_num];
			for (int i = 0; i < col_num; i++) {
				fields[col_num - i - 1] = new char[20];
				memset(fields[col_num - i - 1], 0, 20);
				strcpy(fields[col_num - i - 1], res.fields[i]);
			}

			auto cur_result = &res;
			while (cur_result != nullptr) {
				row_num += cur_result->row_num;
				cur_result = cur_result->next_res;
			}

			cur_result = &res;
			char *** rows = new char**[row_num];
			for (int i = 0; i < row_num; i++) {
				rows[i] = new char*[col_num];
				auto cur_row = cur_result->res[i % MAX_LINES_NUM];
				for (int j = 0; j < col_num; j++) {
					auto cur_type = res.type[j];
					int data_length = 0;
					char *data_text;

					switch (cur_type)
					{
					case ints: {
						char tmp_int[255];
						sprintf(tmp_int, "%d", *(int *)(cur_row[j]));
						data_length = strlen(tmp_int);
						data_text = new char[data_length + 1];
						strcpy(data_text, tmp_int);
						break;
					}
					case floats: {
						char tmp_float[255];
						sprintf(tmp_float, "%f", *(float *)(cur_row[j]));
						data_length = strlen(tmp_float);
						data_text = new char[data_length + 1];
						strcpy(data_text, tmp_float);
						break;
					}
					case chars: {
						data_length = strlen(cur_row[j]);
						data_text = new char[data_length + 1];
						strcpy(data_text, cur_row[j]);
					}
					}

					rows[i][col_num - j - 1] = data_text;
				}
				if (i & MAX_LINES_NUM == 0 && i != 0) {
					cur_result = cur_result->next_res;
				}
			}
			editArea->ShowSelResult(col_num, row_num, fields, rows);
			for (int i = 0; i < col_num; i++) {
				delete[] fields[i];
			}
			delete[] fields;
			Destory_Result(&res);
			return;
		}
	}
	else {
		rc = execute(sql);
	}
	int row_num = 0;
	char**messages;
	switch(rc){
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Success";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Syntax error";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "Function not implemented";
		editArea->ShowMessage(row_num,messages);
	delete[] messages;
		break;
	}
}

RC execute(char * sql){
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
  	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			//case 1:
			////判断SQL语句为select语句

			//break;

			case 2:
			//判断SQL语句为insert语句
			{
				inserts ins = sql_str->sstr.ins;
				auto insertResult = Insert(ins.relName, ins.nValues, ins.values);
				if (insertResult == SUCCESS) {
					AfxMessageBox("Insert success.");
					return SUCCESS;
				}
				else if (insertResult == RM_EOF) {
					AfxMessageBox("Table not exist.");
					return SUCCESS;
				}
				else if (insertResult == RM_INVALIDRECSIZE) {
					AfxMessageBox("Invalid record size.");
					return SQL_SYNTAX;
				}
				else {
					AfxMessageBox("Unknown error.");
					return SQL_SYNTAX;
				}
				break;
			}

			case 3:	
			//判断SQL语句为update语句
			{
				updates upd = sql_str->sstr.upd;
				auto updResult = Update(upd.relName, upd.attrName, &upd.value, upd.nConditions, upd.conditions);
				if (updResult == SUCCESS) {
					AfxMessageBox("Update success.");
					return SUCCESS;
				}
				else if (updResult == RM_EOF) {
					AfxMessageBox("Table not exist.");
					return SUCCESS;
				}
				else {
					AfxMessageBox("Unknown error.");
					return SQL_SYNTAX;
				}
				break;
			}

			case 4:					
			//判断SQL语句为delete语句
			{
				deletes del = sql_str->sstr.del;
				auto delResult = Delete(del.relName, del.nConditions, del.conditions);
				if (delResult == SUCCESS) {
					AfxMessageBox("Delete success.");
					return SUCCESS;
				}
				else if (delResult == RM_EOF) {
					AfxMessageBox("Table not exist.");
					return SUCCESS;
				}
				else {
					AfxMessageBox("Unknown error.");
					return SQL_SYNTAX;
				}
				break;
			}

			case 5:
			//判断SQL语句为createTable语句
			{
				createTable cret = sql_str->sstr.cret;
				auto cretResult = CreateTable(cret.relName, cret.attrCount, cret.attributes);
				if (cretResult == SUCCESS) {
					AfxMessageBox("Create table success.");
					return SUCCESS;
				}
				else {
					AfxMessageBox("Table exists.");
					return SUCCESS;
				}
				break;
			}

			case 6:	
			//判断SQL语句为dropTable语句
			{
				dropTable cret = sql_str->sstr.drt;
				auto dropResult = DropTable(cret.relName);
				if (dropResult == SUCCESS) {
					AfxMessageBox("Drop table success.");
					return SUCCESS;
				}
				else {
					AfxMessageBox("Table not exist.");
					return SUCCESS;
				}
				break;
			}

			case 7:
			//判断SQL语句为createIndex语句
			{
				createIndex crei = sql_str->sstr.crei;
				auto createResult = CreateIndex(crei.indexName, crei.relName, crei.attrName);
				if (createResult == SUCCESS) {
					AfxMessageBox("Create index success.");
					return SUCCESS;
				}
				else if (createResult == RM_EOF) {
					AfxMessageBox("Table not exist.");
					return SUCCESS;
				}
				else {
					AfxMessageBox("Unknown error.");
					return SQL_SYNTAX;
				}
				break;
			}
	
			case 8:	
			//判断SQL语句为dropIndex语句
			{
				dropIndex drpi = sql_str->sstr.dri;
				auto dropResult = DropIndex(drpi.indexName);
				if (dropResult == SUCCESS) {
					AfxMessageBox("Drop index success.");
					return SUCCESS;
				}
				else { 
					AfxMessageBox("Index not exist.");
					return SUCCESS;
				}
				break;
			}
			
			case 9:
			//判断为help语句，可以给出帮助提示
			break;
		
			case 10: 
			//判断为exit语句，可以由此进行退出操作
			break;		
		}
	}else{
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
		return rc;
	}
}

RC OpenSysTables() {
	char full_dbname[255];
	strcpy(full_dbname, sys_dbpath);
	strcat(full_dbname, "/SYSTABLES");
	RM_OpenFile(full_dbname, table_file_handle);
	return SUCCESS;
}

RC OpenSysCols() {
	char full_colname[255];
	strcpy(full_colname, sys_dbpath);
	strcat(full_colname, "/SYSCOLUMNS");
	RM_OpenFile(full_colname, col_file_handle);
	return SUCCESS;
}

RC CloseSysTables() {
	RM_CloseFile(table_file_handle);
	return SUCCESS;
}

RC CloseSysCols() {
	RM_CloseFile(col_file_handle);
	return SUCCESS;
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
	char sys_table_name[255] = "del ";   // todo: in windows may be different
	char sys_col_name[255] = "del ";
	char sys_db_name[255] = "del ";
	char sys_ix_name[255] = "del ";

	strcat(sys_table_name, "SYSTABLES");

	strcat(sys_col_name, "SYSCOLUMNS");

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

RC OpenDB(char *dbpath, char *dbname) {
	table_file_handle = new RM_FileHandle;
	col_file_handle = new RM_FileHandle;
	strcpy(sys_dbpath, dbpath);
	strcpy(sys_dbname, dbname);
	return SUCCESS;
}

RC CloseDB() {
	delete table_file_handle;
	delete col_file_handle;
	memset(sys_dbname, 0, 255);
	return SUCCESS;
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
		memset(curData, 0, COL_ROW_SIZE);
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
	//RID removed_rids[col_num];
	auto removed_rids = new RID[col_num];
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

	char sys_db_name[255] = "del ";
	char sys_ix_name[255] = "del ";

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

	int attr_length;
	int attr_offset;

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
	memcpy(&attr_length, col_record.pData + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE, sizeof(int));
	memcpy(&attr_offset, col_record.pData + TABLENAME_SIZE + ATTRNAME_SIZE + ATTRTYPE_SIZE + ATTRLENGTH_SIZE, sizeof(int));
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

	if (*(cur_row + IX_FLAG_OFFSET) == (char)1) {
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

	// try inserting all existing data
	char full_tab_name[255];
	strcpy(full_tab_name, sys_dbpath);
	strcat(full_tab_name, "/");
	strcat(full_tab_name, sys_dbname);
	strcat(full_tab_name, ".tb.");
	strcat(full_tab_name, relName);

	RM_FileHandle rm_fileHandle;
	RM_FileScan rm_fileScan;
	RM_OpenFile(full_tab_name, &rm_fileHandle);
	OpenScan(&rm_fileScan, &rm_fileHandle, 0, nullptr);
	auto insert_rid = new RID[rm_fileHandle.rm_fileSubHeader->nRecords];
	auto insert_data = new char*[rm_fileHandle.rm_fileSubHeader->nRecords];
	for (int i = 0; i < rm_fileHandle.rm_fileSubHeader->nRecords; i++) {
		insert_data[i] = new char[attr_length];
	}
	int inserted_num = 0;

	while (true) {
		RM_Record record;
		RC result = GetNextRec(&rm_fileScan, &record);
		if (result != SUCCESS) {
			break;
		}
		insert_rid[inserted_num] = record.rid;
		memcpy(insert_data[inserted_num], record.pData + attr_offset, attr_length);
		inserted_num++;
	}
	CloseScan(&rm_fileScan);
	RM_CloseFile(&rm_fileHandle);
	
	auto index_handle = new IX_IndexHandle;
	OpenIndex(full_index_name, index_handle);
	for (int i = 0; i < inserted_num; i++) {
		auto insert_result = InsertEntry(index_handle, insert_data[i], &insert_rid[i]);
		if (insert_result != SUCCESS) {
			DEBUG_LOG("Return Code: %d Index \"%s\" inserting idx fails\n",
				insert_result, full_index_name);
			return IX_EOF;
		}
	}
	CloseIndex(index_handle);
	delete index_handle;
	return SUCCESS;
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

	char full_index_name[255] = "del ";
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

	Value tmp;
	for (int i = 0; i < nValues / 2; i++) {
		tmp = values[i];
		values[i] = values[nValues - i - 1];
		values[nValues - i - 1] = tmp;
	} // values are in reversed order

	int attr_pos = 0;
	//int attr_len[col_num], attr_pos = 0;
	//bool is_index[col_num];
	//char index_name[col_num][255];
	auto attr_len = new int[col_num];
	auto is_index = new bool[col_num];
	auto index_name = new char[col_num][255];
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
		char * cur_data = (char *)values[i].data;
		memcpy(insert_value + attr_offset, cur_data, (size_t)attr_len[i]);
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
			insert_result = InsertEntry(index_handle, (char *)values[i].data, &cur_rid);
			if (insert_result != SUCCESS) {
				DEBUG_LOG("Return Code: %d Index \"%s\" inserting idx fails\n",
					insert_result, index_name[i]);
			}
			ALL_LOG("cur_rid : %d.%d, insert into %s: %s\n", cur_rid.pageNum, cur_rid.slotNum, index_name[i], (char *)values[i].data);
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

	//char *col_name[col_num];
	//int col_length[col_num];
	//int col_offset[col_num];
	//AttrType col_types[col_num];
	//bool col_is_indx[col_num];
	//char *col_indx_name[col_num];
	auto col_name = new char*[col_num];
	auto col_length = new int[col_num];
	auto col_offset = new int[col_num];
	auto col_types = new AttrType[col_num];
	auto col_is_indx = new bool[col_num];
	auto col_indx_name = new char*[col_num];

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
	//RID removed_rid[rm_fileHandle.rm_fileSubHeader->nRecords];
	//char removed_data[rm_fileHandle.rm_fileSubHeader->nRecords][rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
	auto removed_rid = new RID[rm_fileHandle.rm_fileSubHeader->nRecords];
	auto removed_data = new char*[rm_fileHandle.rm_fileSubHeader->nRecords];
	for (int i = 0; i < rm_fileHandle.rm_fileSubHeader->nRecords; i++) {
		removed_data[i] = new char[rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
	}
	int removed_num = 0;

	while (true) {
		RM_Record record;
		RC result = GetNextRec(&rm_fileScan, &record);
		if (result != SUCCESS) {
			break;
		}
		removed_rid[removed_num] = record.rid;
		memcpy(removed_data[removed_num], record.pData, (size_t)rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID));
		removed_num++;
	}
	CloseScan(&rm_fileScan);

	for (int i = 0; i < removed_num; i++) {
		DeleteRec(&rm_fileHandle, &removed_rid[i]);
	}
	RM_CloseFile(&rm_fileHandle);
	ALL_LOG("%d records in %s will be deleted.\n", removed_num, relName);

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

			auto ix_indexHandle = new IX_IndexHandle;
			OpenIndex(full_index_name, ix_indexHandle);
			for (int j = 0; j < removed_num; j++) {
				printf("Deleting %d\n", j);
				DeleteEntry(ix_indexHandle, removed_data[j] + col_offset[i], &removed_rid[j]);
			}
			CloseIndex(ix_indexHandle);
			delete ix_indexHandle;
		}
	}

	for (int i = 0; i < col_num; i++) {
		delete col_name[i];
		delete col_indx_name[i];
	}

	return SUCCESS;
}

RC Update(char *relName, char *attrName, Value *value, int nConditions, Condition *conditions) {
	Con table_condition;
	table_condition.bLhsIsAttr = 1;
	table_condition.bRhsIsAttr = 0;
	table_condition.compOp = EQual;
	table_condition.attrType = chars;
	table_condition.LattrOffset = 0;
	table_condition.LattrLength = 21;
	table_condition.Rvalue = relName;
	int col_num = 0, updated_attr_pos = 0;

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

	//char *col_name[col_num];
	//int col_length[col_num];
	//int col_offset[col_num];
	//AttrType col_types[col_num];
	//bool col_is_indx[col_num];
	//char *col_indx_name[col_num];
	auto col_name = new char*[col_num];
	auto col_length = new int[col_num];
	auto col_offset = new int[col_num];
	auto col_types = new AttrType[col_num];
	auto col_is_indx = new bool[col_num];
	auto col_indx_name = new char*[col_num];

	for (int i = 0; i < col_num; i++) {
		col_name[i] = new char[255];
		col_indx_name[i] = new char[255];
	}

	GetColsInfo(relName, col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name);
	auto cons = convert_conditions(nConditions, conditions, col_num, col_name, col_length, col_offset, col_types);
	for (int i = 0; i < col_num; i++) {
		if (strcmp(col_name[i], attrName) == 0) {
			updated_attr_pos = i;
			break;
		}
	}

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
	//RID removed_rid[rm_fileHandle.rm_fileSubHeader->nRecords];
	//char removed_data[rm_fileHandle.rm_fileSubHeader->nRecords][rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
	//char old_attr_data[rm_fileHandle.rm_fileSubHeader->nRecords][col_length[updated_attr_pos]];
	auto removed_rid = new RID[rm_fileHandle.rm_fileSubHeader->nRecords];
	auto removed_data = new char*[rm_fileHandle.rm_fileSubHeader->nRecords];
	auto old_attr_data = new char*[rm_fileHandle.rm_fileSubHeader->nRecords];
	for (int i = 0; i < rm_fileHandle.rm_fileSubHeader->nRecords; i++) {
		removed_data[i] = new char[rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID)];
		old_attr_data[i] = new char[col_length[updated_attr_pos]];
	}
	int removed_num = 0;

	while (true) {
		RM_Record record;
		RC result = GetNextRec(&rm_fileScan, &record);
		if (result != SUCCESS) {
			break;
		}
		removed_rid[removed_num] = record.rid;
		memcpy(removed_data[removed_num], record.pData, (size_t)rm_fileHandle.rm_fileSubHeader->recordSize - sizeof(bool) - sizeof(RID));
		memcpy(old_attr_data[removed_num], record.pData + col_offset[updated_attr_pos], (size_t)col_length[updated_attr_pos]);
		memcpy(removed_data[removed_num] + col_offset[updated_attr_pos], value->data, (size_t)col_length[updated_attr_pos]);
		removed_num++;
	}
	CloseScan(&rm_fileScan);

	for (int i = 0; i < removed_num; i++) {
		RM_Record updated_record;
		updated_record.bValid = true;
		updated_record.rid = removed_rid[i];
		updated_record.pData = removed_data[i];
		UpdateRec(&rm_fileHandle, &updated_record);
	}
	RM_CloseFile(&rm_fileHandle);

	ALL_LOG("%d records in %s will be updated.\n", removed_num, relName);

	if (col_is_indx[updated_attr_pos]) {
		char full_index_name[255];
		strcpy(full_index_name, sys_dbpath);
		strcat(full_index_name, "/");
		strcat(full_index_name, sys_dbname);
		strcat(full_index_name, ".ix.");
		strcat(full_index_name, relName);
		strcat(full_index_name, ".");
		strcat(full_index_name, col_indx_name[updated_attr_pos]);

		auto ix_indexHandle = new IX_IndexHandle;
		OpenIndex(full_index_name, ix_indexHandle);
		for (int j = 0; j < removed_num; j++) {
			printf("running %d\n", j);
			DeleteEntry(ix_indexHandle, old_attr_data[j], &removed_rid[j]);
			InsertEntry(ix_indexHandle, removed_data[j] + col_offset[updated_attr_pos], &removed_rid[j]);
		}
		CloseIndex(ix_indexHandle);
		delete ix_indexHandle;
	}

	for (int i = 0; i < col_num; i++) {
		delete col_name[i];
		delete col_indx_name[i];
	}

	return SUCCESS;
}

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

RC GetTableInfo(char *relName, int *colNum) {
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
	if (is_existed != SUCCESS) {
		DEBUG_LOG("Return Code: %d Table \"%s\" doesn't exist\n", is_existed, relName);

		CloseScan(&table_scan);
		CloseSysTables();
		return is_existed;
	}
	memcpy(colNum, table_record.pData + TABLENAME_SIZE, sizeof(int));
	CloseScan(&table_scan);
	CloseSysTables();
}

bool CanButtonClick(){//需要重新实现
	//如果当前有数据库已经打开
	return true;
	//如果当前没有数据库打开
	//return false;
}
