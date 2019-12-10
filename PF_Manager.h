#ifndef PF_MANAGER_H_H
#define PF_MANAGER_H_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <windows.h>

#include "RC.h"

#define PF_PAGE_SIZE ((1<<12)-4)
#define PF_FILESUBHDR_SIZE (sizeof(PF_FileSubHeader))
#define PF_BUFFER_SIZE 50//页面缓冲区的大小
#define PF_PAGESIZE (1<<12)
typedef unsigned int PageNum;

typedef struct {
	PageNum pageNum;
	char pData[PF_PAGE_SIZE];
} Page;

typedef struct {
	PageNum pageCount;
	int nAllocatedPages;
} PF_FileSubHeader;

typedef struct {
	bool bOpen;
	int fileDesc;
	Page page;
} PF_PageHandle;

typedef struct {
	bool bopen;
	char *fileName;     // name of file
	int fileDesc;       // description
	Page *pHdrPage;     // head page
	char *pBitmap;      // bitmap in control page 8 bit from right to left
	PF_FileSubHeader *pFileSubHeader;
} PF_FileHandle; // superset

const RC CreateFile(const char *fileName);

const RC OpenFile(char *fileName, PF_FileHandle *fileHandle);

const RC CloseFile(PF_FileHandle *fileHandle);

const RC GetThisPage(PF_FileHandle *fileHandle, PageNum pageNum, PF_PageHandle *pageHandle);

const RC AllocatePage(PF_FileHandle *fileHandle, PF_PageHandle *pageHandle);

const RC GetPageNum(PF_PageHandle *pageHandle, PageNum *pageNum);

const RC GetData(PF_PageHandle *pageHandle, char **pData);

const RC DisposePage(PF_FileHandle *fileHandle, PageNum pageNum);

const RC MarkDirty(PF_FileHandle *fileHandle, PF_PageHandle *pageHandle);

const RC UnpinPage(PF_PageHandle *pageHandle);

#endif