//
// Created by 唐艺峰 on 2018/12/11.
//

#include "RM_Manager.h"

RC GetNextRec(RM_FileScan *rmFileScan, RM_Record *rec) {
    return PF_NOBUF;
}

RC OpenScan(RM_FileScan *rmFileScan, RM_FileHandle *fileHandle, int conNum, Con *conditions) {
    return PF_NOBUF;
}

RC CloseScan(RM_FileScan *rmFileScan) {
    return PF_NOBUF;
}

RC UpdateRec(RM_FileHandle *fileHandle, const RM_Record *rec) {
    return PF_NOBUF;
}

RC DeleteRec(RM_FileHandle *fileHandle, const RID *rid) {

    return PF_NOBUF;
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
        availablePageNum -= bitmapCount;
    } // check whether need new page

    auto pf_pageHandle = new PF_PageHandle;
    int maxRecordsPerPage = fileHandle->rm_fileSubHeader->recordsPerPage;
    int bitmapSize = (int) ceil((double) maxRecordsPerPage / 8.0);
    pf_pageHandle->bOpen = true;

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
    memcpy(newRecord + (sizeof(bool) + sizeof(RID)), pData, sizeof(char) * dataSize);
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
        fullMapPos = newRecordPagePos / 4u;
        char fullMapMask = (char) (1 << (newRecordPagePos % 4));
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

RC GetRec(RM_FileHandle *fileHandle, RID *rid, RM_Record *rec) {
    int aimPagePos = rid->pageNum;
    int aimSlotPos = rid->slotNum;



    return PF_NOBUF;
}

RC RM_CloseFile(RM_FileHandle *fileHandle) {
    CloseFile(fileHandle->pf_fileHandle);
    delete fileHandle->rm_fileSubHeader;
    return SUCCESS;
}

RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle) {
    auto pf_fileHandle = new PF_FileHandle;
    if (OpenFile(fileName, pf_fileHandle) != SUCCESS) {
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
        return fileCreateCode;
    }
    OpenFile(fileName, pf_fileHandle);

    RM_FileSubHeader rm_fileSubHeader;
    rm_fileSubHeader.nRecords = 0;
    rm_fileSubHeader.recordSize = recordSize;
    rm_fileSubHeader.recordsPerPage = PF_PAGE_SIZE / recordSize;

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
