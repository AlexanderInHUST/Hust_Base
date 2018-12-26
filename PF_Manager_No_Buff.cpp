//
// Created by 唐艺峰 on 2018/12/26.
//

#include "PF_Manager_No_Buff.h"

const RC CreateFile(const char *fileName) {
    int fd;
    char *bitmap;
    PF_FileSubHeader *fileSubHeader;
    fd = open(fileName, O_RDWR | O_CREAT | O_EXCL, //  | O_BINARY in windows
              S_IREAD | S_IWRITE);
    if (fd < 0) {
        return PF_EXIST;
    }
    close(fd);
    fd = open(fileName, O_RDWR);
    Page page;
    memset(&page, 0, PF_PAGESIZE);
    bitmap = page.pData + (int) PF_FILESUBHDR_SIZE;
    fileSubHeader = (PF_FileSubHeader *) page.pData;
    fileSubHeader->nAllocatedPages = 1;
    bitmap[0] |= 0x01;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return PF_FILEERR;
    }
    if (write(fd, (char *) &page, sizeof(Page)) != sizeof(Page)) {
        close(fd);
        return PF_FILEERR;
    }
    if (close(fd) < 0) {
        return PF_FILEERR;
    }
    return SUCCESS;
}

const RC OpenFile(char *fileName, PF_FileHandle *fileHandle) {
    int fd;
    PF_FileHandle *pfilehandle = fileHandle;
    if ((fd = open(fileName, O_RDWR)) < 0) // | _O_BINARY in windows
        return PF_FILEERR;
    pfilehandle->bopen = true;
    pfilehandle->fileName = fileName;
    pfilehandle->fileDesc = fd;
    pfilehandle->pHdrPage = new Page;
    if (lseek(fd, 0, SEEK_SET) == -1) {
        delete pfilehandle->pHdrPage;
        close(fd);
        return PF_FILEERR;
    }
    if (read(fd, pfilehandle->pHdrPage, sizeof(Page)) != sizeof(Page)) {
        delete pfilehandle->pHdrPage;
        close(fd);
        return PF_FILEERR;
    }
    pfilehandle->pBitmap = pfilehandle->pHdrPage->pData + PF_FILESUBHDR_SIZE;
    pfilehandle->pFileSubHeader = (PF_FileSubHeader *) pfilehandle->pHdrPage->pData;
    return SUCCESS;
}

const RC CloseFile(PF_FileHandle *fileHandle) {
    if (lseek(fileHandle->fileDesc, 0, SEEK_SET) != 0) {
        return PF_FILEERR;
    }
    if (write(fileHandle->fileDesc, fileHandle->pHdrPage, sizeof(Page)) != sizeof(Page)) {
        return PF_FILEERR;
    }
    delete fileHandle->pHdrPage;
    if (close(fileHandle->fileDesc) < 0)
        return PF_FILEERR;
    return SUCCESS;
}

const RC GetThisPage(PF_FileHandle *fileHandle, PageNum pageNum, PF_PageHandle *pageHandle) {
    int offset;
    PF_PageHandle *pPageHandle = pageHandle;
    if (pageNum > fileHandle->pFileSubHeader->pageCount)
        return PF_INVALIDPAGENUM;
    if ((fileHandle->pBitmap[pageNum / 8] & (1 << (pageNum % 8))) == 0)
        return PF_INVALIDPAGENUM;
    pPageHandle->bOpen = true;
    pPageHandle->fileDesc = fileHandle->fileDesc;
    offset = pageNum * sizeof(Page);
    if (lseek(fileHandle->fileDesc, offset, SEEK_SET) == offset - 1) {
        return PF_FILEERR;
    }
    if ((read(fileHandle->fileDesc, &(pPageHandle->page), sizeof(Page))) != sizeof(Page)) {
        return PF_FILEERR;
    }
    return SUCCESS;
}

const RC AllocatePage(PF_FileHandle *fileHandle, PF_PageHandle *pageHandle) {
    PF_PageHandle *pPageHandle = pageHandle;
    int byte, bit;
    PageNum i;
    if ((fileHandle->pFileSubHeader->nAllocatedPages) <= (fileHandle->pFileSubHeader->pageCount)) {
        for (i = 0; i <= fileHandle->pFileSubHeader->pageCount; i++) {
            byte = i / 8;
            bit = i % 8;
            if (((fileHandle->pBitmap[byte]) & (1 << bit)) == 0) {
                (fileHandle->pFileSubHeader->nAllocatedPages)++;
                fileHandle->pBitmap[byte] |= (1 << bit);
                break;
            }
        }
        if (i <= fileHandle->pFileSubHeader->pageCount) {
            return GetThisPage(fileHandle, i, pageHandle);
        }
    }
    fileHandle->pFileSubHeader->nAllocatedPages++;
    fileHandle->pFileSubHeader->pageCount++;
    byte = fileHandle->pFileSubHeader->pageCount / 8;
    bit = fileHandle->pFileSubHeader->pageCount % 8;
    fileHandle->pBitmap[byte] |= (1 << bit);

    pPageHandle->bOpen = true;
    pPageHandle->fileDesc = fileHandle->fileDesc;
    memset(&(pPageHandle->page), 0, sizeof(Page));
    pPageHandle->page.pageNum = fileHandle->pFileSubHeader->pageCount;

    if (lseek(fileHandle->fileDesc, 0, SEEK_END) == -1) {
        return PF_FILEERR;
    }
    if (write(fileHandle->fileDesc, &(pPageHandle->page), sizeof(Page)) != sizeof(Page)) {
        return PF_FILEERR;
    }
    return SUCCESS;
}

const RC GetPageNum(PF_PageHandle *pageHandle, PageNum *pageNum) {
    if (!pageHandle->bOpen)
        return PF_PHCLOSED;
    *pageNum = pageHandle->page.pageNum;
    return SUCCESS;
}

const RC GetData(PF_PageHandle *pageHandle, char **pData) {
    if (!pageHandle->bOpen)
        return PF_PHCLOSED;
    *pData = pageHandle->page.pData;
    return SUCCESS;
}

const RC DisposePage(PF_FileHandle *fileHandle, PageNum pageNum) {
    char tmp;
    if (pageNum > fileHandle->pFileSubHeader->pageCount) {
        return PF_INVALIDPAGENUM;
    }
    if (((fileHandle->pBitmap[pageNum / 8]) & (1 << (pageNum % 8))) == 0) {
        return PF_INVALIDPAGENUM;
    }
    fileHandle->pFileSubHeader->nAllocatedPages--;
    tmp = (char) (1 << (pageNum % 8));
    fileHandle->pBitmap[pageNum / 8] &= ~tmp;
    return SUCCESS;
}

const RC MarkDirty(PF_FileHandle *fileHandle, PF_PageHandle *pageHandle) {
    int offset = pageHandle->page.pageNum * sizeof(Page);
    if (((fileHandle->pBitmap[pageHandle->page.pageNum / 8]) & (1 << (pageHandle->page.pageNum % 8))) == 0) {
        return SUCCESS;
    }
    if (lseek(pageHandle->fileDesc, offset, SEEK_SET) == offset - 1) {
        return PF_FILEERR;
    }
    if (write(pageHandle->fileDesc, &(pageHandle->page), sizeof(Page)) != sizeof(Page)) {
        return PF_FILEERR;
    }
    return SUCCESS;
}


const RC UnpinPage(PF_PageHandle *pageHandle) {
    return SUCCESS;
}
