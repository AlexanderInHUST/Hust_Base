//
// Created by 唐艺峰 on 2018/12/11.
//

#include "RM_Manager.h"

bool checkConditions(RM_FileScan *rm_fileScan, RM_Record *record) {
    auto conditions = rm_fileScan->conditions;
    auto conNum = rm_fileScan->conNum;
    bool result = true;
    for (int i = 0; i < conNum; i++) {
        auto currentCon = conditions[i];
        char * leftValue, * rightValue;

        if (currentCon.bLhsIsAttr == 0) {
            leftValue = (char *) currentCon.Lvalue;
        } else {
            leftValue = new char[currentCon.LattrLength];
            memcpy(leftValue, record->pData + sizeof(char) * currentCon.LattrOffset, sizeof(char) * currentCon.LattrLength);
        } // load left value

        if (currentCon.bRhsIsAttr == 0) {
            rightValue = (char *) currentCon.Rvalue;
        } else {
            rightValue = new char[currentCon.RattrLength];
            memcpy(rightValue, record->pData + sizeof(char) * currentCon.RattrOffset, sizeof(char) * currentCon.RattrLength);
        } // load right value

        switch (currentCon.attrType) {
            case ints: {
                int lValue = * (int *) leftValue;
                int rValue = * (int *) rightValue;
                switch (currentCon.compOp) {
                    case EQual: {
                        result &= (lValue == rValue);
                        break;
                    }
                    case LEqual: {
                        result &= (lValue <= rValue);
                        break;
                    }
                    case NEqual: {
                        result &= (lValue != rValue);
                        break;
                    }
                    case LessT: {
                        result &= (lValue < rValue);
                        break;
                    }
                    case GEqual: {
                        result &= (lValue >= rValue);
                        break;
                    }
                    case GreatT: {
                        result &= (lValue > rValue);
                        break;
                    }
                    case NO_OP: {
                        break;
                    }
                }
                break;
            }
            case floats: {
                float lValue = * (float *) leftValue;
                float rValue = * (float *) rightValue;
                switch (currentCon.compOp) {
                    case EQual: {
                        result &= floatEqual(lValue, rValue);
                        break;
                    }
                    case LEqual: {
                        result &= (floatEqual(lValue, rValue) | floatLess(lValue, rValue));
                        break;
                    }
                    case NEqual: {
                        result &= (!floatEqual(lValue, rValue));
                        break;
                    }
                    case LessT: {
                        result &= floatLess(lValue, rValue);
                        break;
                    }
                    case GEqual: {
                        result &= (floatEqual(lValue, rValue) | floatLess(rValue, lValue));
                        break;
                    }
                    case GreatT: {
                        result &= floatLess(rValue, lValue);
                        break;
                    }
                    case NO_OP: {
                        break;
                    }
                }
                break;
            }
            case chars: {
                switch (currentCon.compOp) {
                    case EQual: {
                        result &= (strcmp(leftValue, rightValue) == 0);
                        break;
                    }
                    case LEqual: {
                        result &= (strcmp(leftValue, rightValue) <= 0);
                        break;
                    }
                    case NEqual: {
                        result &= (strcmp(leftValue, rightValue) != 0);
                        break;
                    }
                    case LessT: {
                        result &= (strcmp(leftValue, rightValue) < 0);
                        break;
                    }
                    case GEqual: {
                        result &= (strcmp(leftValue, rightValue) >= 0);
                        break;
                    }
                    case GreatT: {
                        result &= (strcmp(leftValue, rightValue) > 0);
                        break;
                    }
                    case NO_OP: {
                        break;
                    }
                }
                break;
            }
        }

        if (currentCon.bLhsIsAttr == 1) {
            delete leftValue;
        }
        if (currentCon.bRhsIsAttr == 1) {
            delete rightValue;
        }
    }
    return result;
}

bool GetNextRID(RM_FileHandle *fileHandle, RID *lastRID, RID *nextRID) {
    auto pf_fileHandle = fileHandle->pf_fileHandle;
    auto allocatedMap = pf_fileHandle->pBitmap;

    PageNum lastPagePos = lastRID->pageNum / 8;
    int lastPageInnerPos = lastRID->pageNum % 8;
    int lastSlotPos = lastRID->slotNum / 8;
    int lastSlotInnerPos = lastRID->slotNum % 8;

    int bitmapSize = (int) ceil((double) fileHandle->rm_fileSubHeader->recordsPerPage / 8.0);
    int allocateMapSize = (int) ceil((double) fileHandle->pf_fileHandle->pFileSubHeader->pageCount / 8.0);

    auto lastDataPage = new PF_PageHandle;
    char * src_data;
    GetThisPage(pf_fileHandle, lastRID->pageNum, lastDataPage);
    GetData(lastDataPage, &src_data);
    auto slotBitmap = src_data + sizeof(int);
    delete lastDataPage;
    // get the slot bitmap in page

    char curBitByte = slotBitmap[lastSlotPos];
    for (int i = lastSlotInnerPos + 1; i < 8; i++) {
        char innerMask = (char) 1 << i;
        if ((curBitByte & innerMask) != 0) {
            nextRID->bValid = true;
            nextRID->pageNum = lastRID->pageNum;
            nextRID->slotNum = lastSlotPos * 8 + i;
            return true;
        }
    } // search current slot byte

    for (int i = lastSlotPos + 1; i < bitmapSize; i++) {
        curBitByte = slotBitmap[i];
        if (curBitByte == 0) {
            continue;
        }
        for (int j = 0; j < 8; j++) {
            char innerMask = (char) 1 << j;
            if ((curBitByte & innerMask) != 0) {
                nextRID->bValid = true;
                nextRID->pageNum = lastRID->pageNum;
                nextRID->slotNum = i * 8 + j;
                return true;
            }
        }
    } // search current page

    char curAllocateBitByte = allocatedMap[lastPagePos];
    for (int i = lastPageInnerPos + 1; i < 8; i++) {
        char innerAllocateMask = (char) 1 << i;
        if ((curAllocateBitByte & innerAllocateMask) != 0) { // find next page
            PageNum aimPageNum = lastPagePos * 8 + i; // possible page
            auto aimPageHandle = new PF_PageHandle;
            GetThisPage(fileHandle->pf_fileHandle, aimPageNum, aimPageHandle);
            char * page_data;
            GetData(aimPageHandle, &page_data); // get the page's data
            char * pageBitmap = page_data + sizeof(int);

            for (int j = 0; j < bitmapSize; j++) {
                curBitByte = pageBitmap[j];
                if (curBitByte == 0) {
                    continue;
                }   // if this byte is empty

                for (int k = 0; k < 8; k++) {
                    char innerMask = (char) 1 << k;
                    if ((curBitByte & innerMask) != 0) {
                        nextRID->bValid = true;
                        nextRID->pageNum = aimPageNum;
                        nextRID->slotNum = j * 8 + k;
                        delete aimPageHandle;
                        return true;
                    }
                }
            }
            delete aimPageHandle;
        }
    } // search current page byte

    for (int i = lastPagePos + 1; i < allocateMapSize; i++) {
        curAllocateBitByte = allocatedMap[i];
        for (int j = 0; j < 8; j++) {
            char innerAllocateMask = (char) 1 << j;
            if ((curAllocateBitByte & innerAllocateMask) != 0) {
                auto aimPageNum = (PageNum) (i * 8u + j);
                auto aimPageHandle = new PF_PageHandle;
                GetThisPage(fileHandle->pf_fileHandle, aimPageNum, aimPageHandle);
                char * page_data;
                GetData(aimPageHandle, &page_data); // get the page's data
                char * pageBitmap = page_data + sizeof(int);

                for (int k = 0; k < bitmapSize; k++) {
                    curBitByte = pageBitmap[k];
                    if (curBitByte == 0) {
                        continue;
                    }   // if this byte is empty

                    for (int l = 0; l < 8; l++) {
                        char innerMask = (char) 1 << l;
                        if ((curBitByte & innerMask) != 0) {
                            nextRID->bValid = true;
                            nextRID->pageNum = aimPageNum;
                            nextRID->slotNum = k * 8 + l;
                            delete aimPageHandle;
                            return true;
                        }
                    }
                }
                delete aimPageHandle;
            }
        }
    } // search rest page
    return false;
}

RC GetNextRec(RM_FileScan *rmFileScan, RM_Record *rec) {
    if (!rmFileScan->bOpen) {
        return RM_FSCLOSED;
    }

    auto rm_fileHandle = rmFileScan->pRMFileHandle;
    int recordSize = rm_fileHandle->rm_fileSubHeader->recordSize;
    int dataSize = recordSize - sizeof(bool) - sizeof(RID);

    RID lastRid;
    lastRid.bValid = true;
    lastRid.pageNum = rmFileScan->pn;
    lastRid.slotNum = rmFileScan->sn;
    RID nextRid;

    while (GetNextRID(rm_fileHandle, &lastRid, &nextRid)) {
        auto nextRecord = new RM_Record;
        GetRec(rm_fileHandle, &nextRid, nextRecord);
        if (checkConditions(rmFileScan, nextRecord)) {
            rmFileScan->pn = nextRid.pageNum;
            rmFileScan->sn = nextRid.slotNum;
            rec->bValid = true;
            rec->rid = nextRid;
            rec->pData = new char[dataSize];
            memcpy(rec->pData, nextRecord->pData, sizeof(char) * dataSize);
            return SUCCESS;
        } else {
            lastRid = nextRid;
        }
    }

    return RM_EOF;
}

RC OpenScan(RM_FileScan *rmFileScan, RM_FileHandle *fileHandle, int conNum, Con *conditions) {
    rmFileScan->bOpen = true;
    rmFileScan->pRMFileHandle = fileHandle;
    rmFileScan->conNum = conNum;
    rmFileScan->conditions = conditions;
    rmFileScan->pn = 2;
    rmFileScan->sn = -1;
    return SUCCESS;
}

RC CloseScan(RM_FileScan *rmFileScan) {
    rmFileScan->bOpen = false;
    rmFileScan->pRMFileHandle = nullptr;
    rmFileScan->conNum = -1;
    rmFileScan->conditions = nullptr;
    rmFileScan->pn = (PageNum) 2;
    rmFileScan->sn = -1;
    return SUCCESS;
}

RC RM_CheckWhetherRecordsExists(RM_FileHandle *fileHandle, RID rid, char ** data, PF_PageHandle ** src_pf_pageHandle) {
    PageNum aimPagePos = rid.pageNum;
    SlotNum aimSlotPos = rid.slotNum;

    auto pf_pageHandle = * src_pf_pageHandle;
    RC openResult = GetThisPage(fileHandle->pf_fileHandle, aimPagePos, pf_pageHandle);
    if (openResult != SUCCESS) {
        return openResult;
    } // check whether the page is allocated

    if (aimSlotPos >= fileHandle->rm_fileSubHeader->recordsPerPage) {
        return RM_INVALIDRID;
    } // check whether the slot pos is beyond the max possible value

    char * src_data;
    GetData(pf_pageHandle, &src_data);
    // get the data pointer

    char * theVeryMap = src_data + sizeof(int) + (aimSlotPos / 8) * sizeof(char);
    char innerMask = (char) 1 << (aimSlotPos % 8u);
    if ((*theVeryMap & innerMask) == 0) {
        return RM_INVALIDRID;
    } // check whether the slot pos is valid

    * data = src_data;
    return SUCCESS;
}

RC UpdateRec(RM_FileHandle *fileHandle, const RM_Record *rec) {
    char * dst_data;
    auto pf_fileHandle = new PF_PageHandle;
    RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, rec->rid, &dst_data, &pf_fileHandle);
    if (checkResult != SUCCESS) {
        return checkResult;
    }

    int recordSize = fileHandle->rm_fileSubHeader->recordSize;
    int dataSize = recordSize - sizeof(bool) - sizeof(RID);
    int bitmapSize = (int) ceil((double) fileHandle->rm_fileSubHeader->recordsPerPage / 8.0);
    memcpy(dst_data + sizeof(int) + sizeof(char) * bitmapSize + sizeof(char) * recordSize * rec->rid.slotNum + sizeof(bool) + sizeof(RID),
           rec->pData, sizeof(char) * dataSize);

    MarkDirty(pf_fileHandle);
    UnpinPage(pf_fileHandle);
    delete pf_fileHandle;
    return SUCCESS;
}

RC DeleteRec(RM_FileHandle *fileHandle, const RID *rid) {
    char * src_data;
    auto pf_pageHandle = new PF_PageHandle;
    RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, *rid, &src_data, &pf_pageHandle);
    if (checkResult != SUCCESS) {
        delete pf_pageHandle;
        return checkResult;
    }

    int recordsNum = -1;
    memcpy(&recordsNum, src_data, sizeof(int));
    recordsNum--;
    bool shouldDeleteFull = recordsNum == fileHandle->rm_fileSubHeader->recordsPerPage - 1;
    bool shouldDelete = recordsNum == 0;

    if (!shouldDelete) {
        char * theVeryMap = src_data + sizeof(int) + (rid->slotNum / 8) * sizeof(char);
        char innerMask = (char) 1 << (rid->slotNum % 8u);
        *theVeryMap &= ~innerMask;
        memcpy(src_data, &recordsNum, sizeof(int));

        MarkDirty(pf_pageHandle);
        UnpinPage(pf_pageHandle);
        // erase mark in page header bitmap
    } else {
        DisposePage(fileHandle->pf_fileHandle, rid->pageNum);
        // dispose that page
    }

    char * head_data;
    auto rm_headerPage = new PF_PageHandle;
    GetThisPage(fileHandle->pf_fileHandle, 1, rm_headerPage);
    GetData(rm_headerPage, &head_data);
    memcpy(&recordsNum, head_data, sizeof(int));
    recordsNum--;
    memcpy(head_data, &recordsNum, sizeof(int));

    if (shouldDeleteFull) {
        char * theVeryMap = head_data + sizeof(RM_FileSubHeader) + (rid->pageNum / 8) * sizeof(char);
        char innerMask = (char) 1 << (rid->pageNum % 8u);
        *theVeryMap &= ~innerMask;
    } // erase mark in full bitmap

    MarkDirty(rm_headerPage);
    UnpinPage(rm_headerPage);

    delete pf_pageHandle;
    delete rm_headerPage;
    return SUCCESS;
}

RC InsertRec(RM_FileHandle *fileHandle, char *pData, RID *rid) { // fixme: pData's size must be datasize
    int availablePageNum = fileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages;
    char *allocatedBitmap = fileHandle->pf_fileHandle->pBitmap;
    char *fullBitmap = fileHandle->header_bitmap;
    int curPos = 0;
    PageNum newRecordPagePos = 0;
    bool needNewPage = true;
    while (availablePageNum != 0) {
        int bitmapCount = count_bit_set((unsigned char) allocatedBitmap[curPos]);
        if (allocatedBitmap[curPos] != fullBitmap[curPos]) {
            char diffMap = allocatedBitmap[curPos] ^fullBitmap[curPos];
            newRecordPagePos = curPos * 8u + least_significant_bit_pos((unsigned char) diffMap);
            needNewPage = false;
            break;
        }
        curPos++;
        availablePageNum -= bitmapCount;
    } // check whether need new page

    auto pf_pageHandle = new PF_PageHandle;
    pf_pageHandle->bOpen = true;
    int maxRecordsPerPage = fileHandle->rm_fileSubHeader->recordsPerPage;
    int bitmapSize = (int) ceil((double) maxRecordsPerPage / 8.0);

    if (needNewPage) {
        AllocatePage(fileHandle->pf_fileHandle, pf_pageHandle);
        GetPageNum(pf_pageHandle, &newRecordPagePos);
    } else {
        GetThisPage(fileHandle->pf_fileHandle, newRecordPagePos, pf_pageHandle);
    } // get the dst page num

    char *dst_data;
    GetData(pf_pageHandle, &dst_data);
    SlotNum newRecordPos = -1;
    int recordsNum = (int) dst_data[0];
    auto recordsMap = new char[bitmapSize]{0};
    memcpy(recordsMap, dst_data + sizeof(int), sizeof(char) * bitmapSize);

    for (int i = 0; i < bitmapSize; i++) {
        if (recordsMap[i] != (char) 0xff) {
            int offset = least_significant_bit_pos((unsigned char) ~recordsMap[i]);
            newRecordPos = i * 8u + offset;
            recordsMap[i] |= (1 << offset);
            break;
        }
    } // find new record position and draw in map

    rid->bValid = true;
    rid->pageNum = newRecordPagePos;
    rid->slotNum = newRecordPos;
    // return value

    int recordSize = fileHandle->rm_fileSubHeader->recordSize;
    int dataSize = recordSize - sizeof(bool) - sizeof(RID);
    auto newRecord = (RM_Record *) (dst_data + sizeof(int) + sizeof(char) * bitmapSize + sizeof(char) * recordSize * newRecordPos);
    newRecord->bValid = true;
    newRecord->rid.bValid = true;
    newRecord->rid.pageNum = newRecordPagePos;
    newRecord->rid.slotNum = newRecordPos;
    memcpy(dst_data + sizeof(int) + sizeof(char) * bitmapSize + sizeof(char) * recordSize * newRecordPos + sizeof(bool) + sizeof(RID),
            pData, sizeof(char) * dataSize);
    recordsNum++;
    // store new data on the page

    memcpy(dst_data, &recordsNum, sizeof(int));
    memcpy(dst_data + sizeof(int), recordsMap, sizeof(char) * bitmapSize);
    // store page header info

    MarkDirty(pf_pageHandle);
    UnpinPage(pf_pageHandle);
    // store and close data page file

    int fullMapPos = -1;
    if (recordsNum == maxRecordsPerPage) {
        fullMapPos = newRecordPagePos / 8u;
        char fullMapMask = (char) (1 << (newRecordPagePos % 8u));
        fileHandle->header_bitmap[fullMapPos] |= fullMapMask;
    } // if this page is full

    fileHandle->rm_fileSubHeader->nRecords++;
    // update global info

    char * head_data;
    auto rm_header_page = new PF_PageHandle;
    GetThisPage(fileHandle->pf_fileHandle, 1, rm_header_page);
    GetData(rm_header_page, &head_data);

    memcpy(head_data, fileHandle->rm_fileSubHeader, sizeof(RM_FileSubHeader));
    if (fullMapPos != -1) {
        memcpy(head_data + sizeof(RM_FileSubHeader) + sizeof(char) * fullMapPos, &fileHandle->header_bitmap[fullMapPos], sizeof(char));
    } // if full has been updated

    MarkDirty(rm_header_page);
    UnpinPage(rm_header_page);

    delete pf_pageHandle;
    delete rm_header_page;
    delete[] recordsMap;
    return SUCCESS;
}

RC GetRec(RM_FileHandle *fileHandle, RID *rid, RM_Record *rec) {    // fixme : rec's memory's going to be allocated inside
    char * src_data;
    auto pf_fileHandle = new PF_PageHandle;
    RC checkResult = RM_CheckWhetherRecordsExists(fileHandle, *rid, &src_data, &pf_fileHandle);
    if (checkResult != SUCCESS) {
        delete pf_fileHandle;
        return checkResult;
    }

    int recordSize = fileHandle->rm_fileSubHeader->recordSize;
    int dataSize = recordSize - sizeof(bool) - sizeof(RID);
    int bitmapSize = (int) ceil((double) fileHandle->rm_fileSubHeader->recordsPerPage / 8.0);

    rec->bValid = true;
    rec->rid.bValid = true;
    rec->rid.pageNum = rid->pageNum;
    rec->rid.slotNum = rid->slotNum;
    rec->pData = new char[dataSize];
    memcpy(rec->pData, src_data + sizeof(int) + bitmapSize * sizeof(char) + rid->slotNum * recordSize * sizeof(char) + sizeof(bool) + sizeof(RID),
           sizeof(char) * dataSize);
    // load data

    delete pf_fileHandle;
    return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle) {
    CloseFile(fileHandle->pf_fileHandle);
    fileHandle->bOpen = false;
    delete fileHandle->pf_fileHandle;
    delete fileHandle->rm_fileSubHeader;
    return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle) {
    auto pf_fileHandle = new PF_FileHandle;
    if (OpenFile(fileName, pf_fileHandle) != SUCCESS) {
        delete pf_fileHandle;
        return PF_FILEERR;
    }

    auto rm_fileSubHeader = new RM_FileSubHeader;
    auto pf_pageHandle = new PF_PageHandle;
    GetThisPage(pf_fileHandle, 1, pf_pageHandle);
    char *src_data;
    GetData(pf_pageHandle, &src_data);
    memcpy(rm_fileSubHeader, src_data, sizeof(RM_FileSubHeader));

    fileHandle->bOpen = true;
    fileHandle->rm_fileSubHeader = rm_fileSubHeader;
    fileHandle->pf_fileHandle = pf_fileHandle;
    fileHandle->header_bitmap = src_data + sizeof(RM_FileSubHeader);

    delete pf_pageHandle;
    return SUCCESS;
}

RC RM_CreateFile(char *fileName, int recordSize) {
    auto pf_fileHandle = new PF_FileHandle();
    RC fileCreateCode = CreateFile(fileName);
    if (fileCreateCode != SUCCESS) {
        delete pf_fileHandle;
        return fileCreateCode;
    }
    OpenFile(fileName, pf_fileHandle);

    RM_FileSubHeader rm_fileSubHeader;
    rm_fileSubHeader.nRecords = 0;
    rm_fileSubHeader.recordSize = recordSize;
    rm_fileSubHeader.recordsPerPage = (PF_PAGE_SIZE - 8) / (recordSize + 1);

    auto pf_pageHandle = new PF_PageHandle();
    pf_pageHandle->bOpen = true;

    AllocatePage(pf_fileHandle, pf_pageHandle);
    char *dst_data;
    char initial_map = 0x3;
    GetData(pf_pageHandle, &dst_data);
    memcpy(dst_data, &rm_fileSubHeader, sizeof(RM_FileSubHeader));
    memcpy(dst_data + sizeof(RM_FileSubHeader), &initial_map, sizeof(char));

    MarkDirty(pf_pageHandle);
    UnpinPage(pf_pageHandle);
    CloseFile(pf_fileHandle);

    delete pf_fileHandle;
    delete pf_pageHandle;
    return SUCCESS;
}