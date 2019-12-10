#include "StdAfx.h"
#include "QU_Manager.h"

void Init_Result(SelResult * res) {
	res->next_res = nullptr;
}

void Destory_Result(SelResult * res) {
	for (int i = 0; i < res->row_num; i++) {
		for (int j = 0; j < res->col_num; j++) {
			delete[] res->res[i][j];
		}
		delete[] res->res[i];
	}
	if (res->next_res != nullptr) {
		Destory_Result(res->next_res);
	}
}

RC Query(char * sql, SelResult * res) {
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX

	if (rc == SQL_SYNTAX) {
		return SQL_SYNTAX;
	}
	auto sel = sql_str->sstr.sel;
	return Select(sel.nSelAttrs, sel.selAttrs, sel.nRelations, sel.relations, sel.nConditions, sel.conditions, res);
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
	set->data = new char *[num];
	set->num = num;
	for (int i = 0; i < num; i++) {
		set->is_alive[i] = true;
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
	set->size = num;
}

void InsertOneRid(RIDSets * set, int pos, RID aim) {
	set->rids[pos] = aim;
	set->status[pos] = 2;
}

void MakeAllInactive(RIDSets * set) {
	for (int i = 0; i < set->size; i++) {
		if (set->status[i] == 2) {
			set->status[i] = 1;
		}
	}
}

void MakeAllClear(RIDSets * set) {
	for (int i = 0; i < set->size; i++) {
		if (set->status[i] == 1) {
			set->status[i] = 0;
			set->num--;
		}
	}
}

bool MakeOneActive(RIDSets * set, RID aim) {
	for (int i = 0; i < set->size; i++) {
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

void DestroyRIDSets(RIDSets * set) {
	delete[] set->rids;
	delete[] set->status;
}

bool GetAttrOffsetLength(char * rel_name, char * attr_name, int nRelations, char ** relations,
	int * col_num, char *** col_name, int ** col_length, int ** col_offset, AttrType ** col_types, int * table_offset,
	int * aim_offset, int * aim_length, AttrType * aim_type) {
	int aim_relation_pos = -1, aim_attr_pos = -1;
	if (rel_name != NULL) {
		for (int i = 0; i < nRelations; i++) {
			if (strcmp(relations[i], rel_name) == 0) {
				aim_relation_pos = i;
				break;
			}
		}
	}// whether has specific relation requirement

	bool is_found = false;
	if (aim_relation_pos == -1) {
		for (int i = 0; i < nRelations; i++) {
			for (int j = 0; j < col_num[i]; j++) {
				if (strcmp(col_name[i][j], attr_name) == 0) {
					aim_relation_pos = i;
					aim_attr_pos = j;
					is_found = true;
					break;
				}
			}
			if (is_found) {
				break;
			}
		}
		if (!is_found) {
			return false; // no such attr at all
		}
	}
	else {
		for (int i = 0; i < col_num[aim_relation_pos]; i++) {
			if (strcmp(col_name[aim_relation_pos][i], attr_name) == 0) {
				aim_attr_pos = i;
				is_found = true;
				break;
			}
		}
		if (!is_found) {
			return false;
		}
	} // find out where they are

	*aim_offset = table_offset[aim_relation_pos] + col_offset[aim_relation_pos][aim_attr_pos];
	*aim_length = col_length[aim_relation_pos][aim_attr_pos];
	*aim_type = col_types[aim_relation_pos][aim_attr_pos];
	return true;
}

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult *res) {
	auto can_be_reduced = new char[nConditions]; // 0 -> nothing 1 -> reduced 2 -> deleted
	auto has_be_loaded = new bool[nRelations];

	auto col_num = new int[nRelations];
	auto col_name = new char **[nRelations];
	auto col_length = new int *[nRelations];
	auto col_offset = new int *[nRelations];
	auto col_types = new AttrType *[nRelations];
	auto col_is_indx = new bool *[nRelations];
	auto col_indx_name = new char **[nRelations];

	auto rid_sets = new RIDSets[nRelations];
	auto table_offset = new int[nRelations + 1];
	auto table_value_num = new int[nRelations];
	auto table_round_num = new int[nRelations];

	for (int i = 0; i < nRelations; i++) {
		RC table_exist = GetTableInfo(relations[i], &col_num[i]);
		int cur_col_num = col_num[i];
		if (table_exist == TABLE_NOT_EXIST) {
			return SQL_SYNTAX;
		}

		has_be_loaded[i] = false;
		col_name[i] = new char *[cur_col_num];
		col_length[i] = new int[cur_col_num];
		col_offset[i] = new int[cur_col_num];
		col_types[i] = new AttrType[cur_col_num];
		col_is_indx[i] = new bool[cur_col_num];
		col_indx_name[i] = new char *[cur_col_num];
		for (int j = 0; j < cur_col_num; j++) {
			col_name[i][j] = new char[255];
			col_indx_name[i][j] = new char[255];
		}
		GetColsInfo(relations[i], col_name[i], col_types[i], col_length[i], col_offset[i], col_is_indx[i], col_indx_name[i]);
	} // get all related col info

	for (int i = 0; i < nConditions; i++) {
		auto cur_con = conditions[i];
		if (cur_con.bRhsIsAttr == 0 && cur_con.bLhsIsAttr == 0) {
			can_be_reduced[i] = 2; // this condition could be deleted
		}
		else if (!(cur_con.bRhsIsAttr == 1 && cur_con.bLhsIsAttr == 1)) {
			can_be_reduced[i] = 1; // this condition could be reduced
		}
		else {
			can_be_reduced[i] = 0; // we cannot do anything at least right now
			continue;
		} // to check whether this condition could be reduced

		if (can_be_reduced[i] == 1) {
			char * attr_name, *rel_name;
			AttrType value_type;
			char * value;

			if (cur_con.bLhsIsAttr == 1) {
				rel_name = cur_con.lhsAttr.relName;
				attr_name = cur_con.lhsAttr.attrName;

				value_type = cur_con.rhsValue.type;
				value = (char *)cur_con.rhsValue.data;
			}
			else {
				rel_name = cur_con.rhsAttr.relName;
				attr_name = cur_con.rhsAttr.attrName;

				value_type = cur_con.lhsValue.type;
				value = (char *)cur_con.lhsValue.data;
			} // initialize important data

			bool is_found = false;
			bool is_duplicate = false;
			bool is_done = false;
			int aim_attr_pos = 0, aim_relation_pos = 0;
			for (int j = 0; j < nRelations; j++) {
				for (int col_pos = 0; col_pos < col_num[j]; col_pos++) {
					if (strcmp(attr_name, col_name[j][col_pos]) == 0) {
						if (rel_name != NULL && strcmp(rel_name, relations[j]) == 0) {
							aim_relation_pos = j;
							aim_attr_pos = col_pos;
							is_done = true;
							break; // if one's rel and col name both equal, it is the col we need.
						}
						is_duplicate = is_found; // would be true when found the 2nd one
						is_found = true;
						if (!is_duplicate) { // record the first info (there should be sql_syntax error when 2nd time)
							aim_relation_pos = j;
							aim_attr_pos = col_pos;
						}
					}
				}
				if (is_done) {
					break;
				}
			}
			if ((!is_done && is_duplicate) || (!is_found && !is_done)) {
				return SQL_SYNTAX;  // cannot tell which attr it is.
			} // to find where is the attr

			if (value_type != col_types[aim_relation_pos][aim_attr_pos]) {
				return SQL_SYNTAX;
			} // if they cannot be compared

			if (col_is_indx[aim_relation_pos][aim_attr_pos] && cur_con.op != NEqual) {
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
				}
				else {
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
			}
			else {
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
						if (result != SUCCESS) {
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
						if (result != SUCCESS) {
							break;
						}
						InsertOneRid(&rid_sets[aim_relation_pos], rid_count, record.rid);
						rid_count++;
					}
				}
				else {
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
		}
		else {
			auto converted_con = convert_conditions(1, &cur_con, 0, nullptr, nullptr, nullptr, nullptr);
			RM_FileScan dump_scan;
			dump_scan.conditions = converted_con;
			dump_scan.conNum = 1;
			if (checkConditions(&dump_scan, nullptr)) {
				continue;
			}
			else {
				int result_count = 0;
				auto cur_result = res;
				cur_result->col_num = nSelAttrs;
				cur_result->row_num = 0;

				auto result_attr_offset = new int[nSelAttrs];
				auto result_attr_length = new int[nSelAttrs];
				auto result_attr_types = new AttrType[nSelAttrs];
				bool is_valid = true;
				for (int i = 0; i < nSelAttrs; i++) {
					is_valid &= GetAttrOffsetLength(selAttrs[i]->relName, selAttrs[i]->attrName, nRelations, relations, col_num, col_name, col_length, col_offset, col_types, table_offset, &result_attr_offset[i], &result_attr_length[i], &result_attr_types[i]);
				}
				if (!is_valid) {
					return SQL_SYNTAX; // no such attr
				}

				for (int j = 0; j < nSelAttrs; j++) {
					cur_result->type[j] = result_attr_types[j];
					cur_result->length[j] = result_attr_length[j];
					strcpy(cur_result->fields[j], selAttrs[j]->attrName);
				} // initialize first result

				delete[] result_attr_offset;
				delete[] result_attr_length;
				delete[] result_attr_types;
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
				if (result != SUCCESS) {
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
				if (result != SUCCESS) {
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
			table_round_num[i] = 1;
		}
		else {
			table_offset[i] = col_length[i - 1][col_num[i - 1] - 1] + col_offset[i - 1][col_num[i - 1] - 1] + table_offset[i - 1];
			table_round_num[i] = table_round_num[i - 1] * rid_sets[i - 1].num;
		} // get table offset for Descartes
	} // get rest possible result
	table_offset[nRelations] = col_length[nRelations - 1][col_num[nRelations - 1] - 1] + col_offset[nRelations - 1][col_num[nRelations - 1] - 1] + table_offset[nRelations - 1];

	for (int i = nRelations - 1; i >= 0; i--) {
		if (i == nRelations - 1) {
			table_value_num[i] = 1;
		}
		else {
			table_value_num[i] = rid_sets[i + 1].num * table_value_num[i + 1];
		}
	} // get the number of each value

	ResultsSets resultsSets;
	InitializeResultsSet(&resultsSets, table_value_num[0] * rid_sets[0].num, table_offset[nRelations]);
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

		for (int round = 0; round < table_round_num[i]; round++) {
			for (int j = 0; j < rid_sets[i].size; j++) {
				if (rid_sets[i].status[j] != 2) {
					continue;
				}
				auto cur_rid = rid_sets[i].rids[j];
				RM_Record cur_record;
				GetRec(&rm_fileHandle, &cur_rid, &cur_record);
				for (int k = 0; k < table_value_num[i]; k++) {
					memcpy(resultsSets.data[line_num] + table_offset[i], cur_record.pData, (size_t)(col_offset[i][col_num[i] - 1] + col_length[i][col_num[i] - 1]));
					line_num++;
				}
			}
		}
		RM_CloseFile(&rm_fileHandle);
	} // make up the whole set

	RM_Record dump_record;
	for (int i = 0; i < nConditions; i++) {
		if (can_be_reduced[i] == 0) {
			int left_attr_offset, left_attr_length;
			int right_attr_offset, right_attr_length;
			AttrType left_attr_type, right_attr_type;
			auto cur_con = conditions[i];
			bool is_valid = true;
			is_valid &= GetAttrOffsetLength(cur_con.lhsAttr.relName, cur_con.lhsAttr.attrName, nRelations, relations, col_num, col_name, col_length, col_offset, col_types, table_offset, &left_attr_offset, &left_attr_length, &left_attr_type);
			is_valid &= GetAttrOffsetLength(cur_con.rhsAttr.relName, cur_con.rhsAttr.attrName, nRelations, relations, col_num, col_name, col_length, col_offset, col_types, table_offset, &right_attr_offset, &right_attr_length, &right_attr_type);
			is_valid &= (left_attr_type == right_attr_type);
			if (!is_valid) {
				return SQL_SYNTAX; // no such attr
			}

			Con dump_con;
			dump_con.bLhsIsAttr = 1;
			dump_con.LattrOffset = left_attr_offset;
			dump_con.LattrLength = left_attr_length;
			dump_con.bRhsIsAttr = 1;
			dump_con.RattrOffset = right_attr_offset;
			dump_con.RattrLength = right_attr_length;
			dump_con.attrType = left_attr_type;
			dump_con.compOp = cur_con.op; // prepare the condition

			RM_FileScan dump_scan;
			dump_scan.conNum = 1;
			dump_scan.conditions = &dump_con;

			for (int j = 0; j < resultsSets.num; j++) {
				if (resultsSets.is_alive[j]) {
					dump_record.pData = resultsSets.data[j];
					resultsSets.is_alive[j] = checkConditions(&dump_scan, &dump_record);
				}
			}
		}
	} // check all conditions

	for (int i = 0; i < nSelAttrs; i++) {
		bool is_done = false, is_duplicate = false, is_found = false;
		for (int j = 0; j < nRelations; j++) {
			for (int col_pos = 0; col_pos < col_num[j]; col_pos++) {
				auto attr_name = selAttrs[i]->attrName;
				auto rel_name = selAttrs[i]->relName;
				if (strcmp(attr_name, col_name[j][col_pos]) == 0) {
					if (rel_name != NULL && strcmp(rel_name, relations[j]) == 0) {
						is_done = true;
						break; // if one's rel and col name both equal, it is the col we need.
					}
					is_duplicate = is_found; // would be true when found the 2nd one
					is_found = true;
				}
			}
			if (is_done) {
				break;
			}
		}
		if ((!is_done && is_duplicate) || (!is_found && !is_done)) {
			return SQL_SYNTAX;  // cannot tell which attr it is.
		} // to find where is the attr
	}

	auto result_attr_offset = new int[nSelAttrs];
	auto result_attr_length = new int[nSelAttrs];
	auto result_attr_types = new AttrType[nSelAttrs];
	bool is_valid = true;
	for (int i = 0; i < nSelAttrs; i++) {
		is_valid &= GetAttrOffsetLength(selAttrs[i]->relName, selAttrs[i]->attrName, nRelations, relations, col_num, col_name, col_length, col_offset, col_types, table_offset, &result_attr_offset[i], &result_attr_length[i], &result_attr_types[i]);
	}
	if (!is_valid) {
		return SQL_SYNTAX; // no such attr
	}

	int result_count = 0;
	auto cur_result = res;
	cur_result->col_num = nSelAttrs;
	cur_result->row_num = 0;
	for (int j = 0; j < nSelAttrs; j++) {
		cur_result->type[j] = result_attr_types[j];
		cur_result->length[j] = result_attr_length[j];
		strcpy(cur_result->fields[j], selAttrs[j]->attrName);
	} // initialize first result

	for (int i = 0; i < resultsSets.num; i++) {
		if (result_count == 0 && i != 0) {
			cur_result->col_num = nSelAttrs;
			cur_result->row_num = 0;
			for (int j = 0; j < nSelAttrs; j++) {
				cur_result->type[j] = result_attr_types[j];
				cur_result->length[j] = result_attr_length[j];
				strcpy(cur_result->fields[j], selAttrs[j]->attrName);
			} // initialize current result
		}

		if (!resultsSets.is_alive[i]) {
			continue;
		}
		else {
			cur_result->res[result_count] = new char *[nSelAttrs];
			for (int j = 0; j < nSelAttrs; j++) {
				cur_result->res[result_count][j] = new char[result_attr_length[j]];
				memcpy(cur_result->res[result_count][j], resultsSets.data[i] + result_attr_offset[j], (size_t)result_attr_length[j]);
			}
			result_count++;
			cur_result->row_num++;
		}

		if (result_count == MAX_LINES_NUM) {
			auto next_res = new SelResult;
			Init_Result(next_res);
			result_count = 0;
			cur_result->next_res = next_res;
			cur_result = next_res;
		}
	}

	DestroyColsInfo(col_name, col_types, col_length, col_offset, col_is_indx, col_indx_name, nRelations, col_num);
	DestroyResultsSet(&resultsSets);
	DestroyRIDSets(rid_sets);

	delete[] result_attr_offset;
	delete[] result_attr_length;
	delete[] result_attr_types;

	delete[] col_num;
	delete[] col_name;
	delete[] col_length;
	delete[] col_offset;
	delete[] col_types;
	delete[] col_is_indx;
	delete[] col_indx_name;

	delete[] can_be_reduced;
	delete[] has_be_loaded;
	delete[] rid_sets;
	delete[] table_offset;
	delete[] table_value_num;
	return SUCCESS;
}