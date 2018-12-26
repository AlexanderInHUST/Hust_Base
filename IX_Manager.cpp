//
// Created by 唐艺峰 on 2018/12/13.
//

#include "IX_Manager.h"

RC CreateIndex(const char *fileName, AttrType attrType, int attrLength) {
    auto pf_fileHandle = new PF_FileHandle();
    RC fileCreateCode = CreateFile(fileName);
    if (fileCreateCode != SUCCESS) {
        delete pf_fileHandle;
        return fileCreateCode;
    }
    OpenFile((char *) fileName, pf_fileHandle);

    int keys_size = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + attrLength);

    auto ix_fileHeader = new IX_FileHeader;
    ix_fileHeader->attrLength = attrLength;
    ix_fileHeader->keyLength = attrLength + sizeof(RID);
    ix_fileHeader->attrType = attrType;
    ix_fileHeader->rootPage = 2;
    ix_fileHeader->first_leaf = 2;
//    ix_fileHeader->order = keys_size - 2; // debug here!
    ix_fileHeader->order = 4;
    // -1 -> for extra child in children list; -1 -> for possible insert place

    auto ix_node = new IX_Node;
    ix_node->is_leaf = true;
    ix_node->keynum = 0;
    ix_node->parent = 0;
    ix_node->brother = 0;
    ix_node->keys_offset = sizeof(IX_FileHeader) + sizeof(IX_Node);
    ix_node->rids_offset = ix_node->keys_offset + keys_size * (sizeof(RID) + attrLength);

    auto pf_pageHandle = new PF_PageHandle();
    pf_pageHandle->bOpen = true;
    AllocatePage(pf_fileHandle, pf_pageHandle);
    char *dst_data;
    GetData(pf_pageHandle, &dst_data);
    memcpy(dst_data, ix_fileHeader, sizeof(IX_FileHeader));
    MarkDirty(pf_fileHandle, pf_pageHandle);
    UnpinPage(pf_pageHandle);
    delete pf_pageHandle;
    // create header page

    pf_pageHandle = new PF_PageHandle();
    pf_pageHandle->bOpen = true;
    AllocatePage(pf_fileHandle, pf_pageHandle);
    GetData(pf_pageHandle, &dst_data);
    memcpy(dst_data + sizeof(IX_FileHeader), ix_node, sizeof(IX_Node));
    MarkDirty(pf_fileHandle, pf_pageHandle);
    UnpinPage(pf_pageHandle);
    // create first page

    CloseFile(pf_fileHandle);

    delete pf_fileHandle;
    delete pf_pageHandle;
    delete ix_fileHeader;
    delete ix_node;
    return SUCCESS;
}

RC OpenIndex(const char *fileName, IX_IndexHandle *indexHandle) {
    auto pf_fileHandle = new PF_FileHandle();
    if (OpenFile((char *) fileName, pf_fileHandle) != SUCCESS) {
        delete pf_fileHandle;
        return PF_FILEERR;
    }

    indexHandle->headerPage = new PF_PageHandle;
    GetThisPage(pf_fileHandle, 1, indexHandle->headerPage);
    char *src_data;
    GetData(indexHandle->headerPage, &src_data);

    indexHandle->bOpen = true;
    indexHandle->fileHandle =  pf_fileHandle;
    indexHandle->fileHeader = (IX_FileHeader *) src_data;
    return SUCCESS;
}

RC CloseIndex(IX_IndexHandle *indexHandle) {
    CloseFile(indexHandle->fileHandle);
    indexHandle->bOpen = false;
    delete indexHandle->fileHandle;
    delete indexHandle->headerPage;
    return SUCCESS;
}

PageNum createNewNode(IX_IndexHandle *indexHandle) {
    auto pf_fileHandle = indexHandle->fileHandle;
    auto pf_pageHandle = new PF_PageHandle();
    pf_pageHandle->bOpen = true;
    RC result = AllocatePage(pf_fileHandle, pf_pageHandle);
    if (result != SUCCESS) {
        printf("Error code: %d when creating new node\n", result);
        exit(1);
    }

    int keys_size = (PF_PAGE_SIZE - sizeof(IX_FileHeader) - sizeof(IX_Node)) / (2 * sizeof(RID) + indexHandle->fileHeader->attrLength);

    auto ix_node = new IX_Node;
    ix_node->is_leaf = false;
    ix_node->keynum = 0;
    ix_node->parent = 0;
    ix_node->brother = 0;
    ix_node->keys_offset = sizeof(IX_FileHeader) + sizeof(IX_Node);
    ix_node->rids_offset = ix_node->keys_offset + indexHandle->fileHeader->keyLength * keys_size;

    char *dst_data;
    GetData(pf_pageHandle, &dst_data);
    memcpy(dst_data + sizeof(IX_FileHeader), ix_node, sizeof(IX_Node));

    PageNum newPageNum = 0;
    GetPageNum(pf_pageHandle, &newPageNum);

    MarkDirty(pf_fileHandle, pf_pageHandle);
    delete pf_pageHandle;
    delete ix_node;
    return newPageNum;
}

IX_Node * getIxNode(IX_IndexHandle *indexHandle, PageNum pageNum, PF_PageHandle * pf_pageHandle) {
    RC result = GetThisPage(indexHandle->fileHandle, pageNum, pf_pageHandle);
    char *src_data;
    GetData(pf_pageHandle, &src_data);
    return (IX_Node *) (src_data + sizeof(IX_FileHeader));
}

int compareKey(char *key1, char *key2, AttrType attrType, int attrLength) {
    switch (attrType) {
        case ints: {
            int k1 = * (int *) key1;
            int k2 = * (int *) key2;
            if (k1 != k2) {
                return k1 > k2 ? 1 : -1;
            }
            break;
        }
        case floats: {
            float k1 = * (float *) key1;
            float k2 = * (float *) key2;
            if (!floatEqual(k1, k2)) {
                return floatLess(k2, k1) ? 1 : -1;
            }
            break;
        }
        case chars: {
            int strcmpResult = strcmp(key1, key2);
            if (strcmpResult != 0) {
                return strcmpResult;
            }
            break;
        }
    }
    RID *rid1 = (RID *) (key1 + (sizeof(char) * attrLength));
    RID *rid2 = (RID *) (key2 + (sizeof(char) * attrLength));
    if (rid1->pageNum != rid2->pageNum) {
        return rid1->pageNum > rid2->pageNum ? 2 : -2;
    } else if (rid1->slotNum != rid2->slotNum) {
        return rid1->slotNum > rid2->slotNum ? 2 : -2;
    } else {
        return 0;
    }
}

PageNum findKey(IX_IndexHandle *indexHandle, AttrType attrType, int attrLength, int keyLength, char *key) {
    PageNum currentPage = indexHandle->fileHeader->rootPage;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    char *src_data;

    while (!currentNode->is_leaf) {
        bool isFound = false;
        GetData(currentNodePage, &src_data);
        // Get that data

        char *keys = src_data + currentNode->keys_offset;
        char *rids = src_data + currentNode->rids_offset;
        for (int i = 0; i < currentNode->keynum; i++) {
            char *curKey = keys + sizeof(char) * i * keyLength;
            if (compareKey(curKey, key, attrType, attrLength) > 0) {
                PageNum curChild = * (PageNum *) (rids + sizeof(PageNum) * i);
                UnpinPage(currentNodePage);
                currentNode = getIxNode(indexHandle, curChild, currentNodePage);
                currentPage = curChild;
                isFound = true;
                break;
            }
        }
        if (!isFound) {
            PageNum curChild = * (PageNum *) (rids + sizeof(PageNum) * currentNode->keynum);
            UnpinPage(currentNodePage);
            currentNode = getIxNode(indexHandle, curChild, currentNodePage);
            currentPage = curChild;
        }
    }

    UnpinPage(currentNodePage);
    delete currentNodePage;
    return currentPage;
}

bool checkConditions(IX_IndexScan *indexScan, char * check_value) {
    auto keyLength = indexScan->pIXIndexHandle->fileHeader->keyLength;
    auto attrLength = indexScan->pIXIndexHandle->fileHeader->attrLength;
    auto attrType = indexScan->pIXIndexHandle->fileHeader->attrType;
    char comparedKey[keyLength];
    memcpy(comparedKey, indexScan->value, attrLength);
    memset(comparedKey + attrLength, 0, sizeof(RID));
    int compare_result = compareKey(check_value, comparedKey, attrType, attrLength);
    compare_result = (compare_result == -2 || compare_result == 2) ? 0 : compare_result;

    switch (indexScan->compOp) {
        case EQual: {
            return compare_result == 0;
        }
        case LEqual: {
            return compare_result <= 0;
        }
        case NEqual: {
            return compare_result != 0;
        }
        case LessT: {
            return compare_result < 0;
        }
        case GEqual: {
            return compare_result >= 0;
        }
        case GreatT: {
            return compare_result > 0;
        }
        case NO_OP: {
            return true;
        }
    }
}

RC findStartKey(IX_IndexScan *indexScan, char *key) {
    auto indexHandle = indexScan->pIXIndexHandle;
    auto keyLength = indexHandle->fileHeader->keyLength;
    auto attrType = indexHandle->fileHeader->attrType;
    auto attrLength = indexHandle->fileHeader->attrLength;

    PageNum currentPage = indexHandle->fileHeader->rootPage;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    char *src_data;
    UnpinPage(currentNodePage);
    delete currentNodePage;

    if (indexScan->compOp == LessT || indexScan->compOp == LEqual || indexScan->compOp == NO_OP) {
        while (!currentNode->is_leaf) {
            GetThisPage(indexHandle->fileHandle, currentPage, currentNodePage);
            GetData(currentNodePage, &src_data);
            char *rids = src_data + currentNode->rids_offset;
            memcpy(&currentPage, rids, sizeof(PageNum));
            currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
        }
        indexScan->pnNext = currentPage;
        indexScan->ridIx = 0;
        return INDEX_EXIST;
    }

    currentPage = findKey(indexScan->pIXIndexHandle, attrType, attrLength, keyLength, key);
    currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    GetThisPage(indexHandle->fileHandle, currentPage, currentNodePage);
    GetData(currentNodePage, &src_data);

    char *keys = src_data + currentNode->keys_offset;
    for (int i = 0; i < currentNode->keynum; i++) {
        char *cur_key = keys + keyLength * i;
        if (compareKey(cur_key, key, attrType, attrLength) >= 0) {
            indexScan->pnNext = currentPage;
            indexScan->ridIx = i;
            return INDEX_EXIST;
        }
    }
    indexScan->pnNext = currentNode->brother;
    indexScan->ridIx = 0;
    return INDEX_NOT_EXIST;
}

void addToList(int keyLength, char *keyList, int list_size, char *key, int pos) {
    if (keyLength == sizeof(PageNum)) {
        PageNum tmp = 0;
        memcpy(&tmp, key, sizeof(PageNum));
        if (tmp == 0) {
            printf("??");
        }
    }

    for (int i = list_size - 1; i >= pos; i--) {
        char *curKey = keyList + keyLength * i;
        memcpy(curKey + keyLength, curKey, sizeof(char) * keyLength);
    }
    memcpy(keyList + keyLength * pos, key, sizeof(char) * keyLength);
}

void removeFromList(int keyLength, char *keyList, int list_size, char *key, int pos) {    // fixme: key's memory must be allocated
    memcpy(key, keyList + keyLength * pos, sizeof(char) * keyLength);
    for (int i = pos + 1; i < list_size; i++) {
        char *curKey = keyList + keyLength * i;
        memcpy(curKey - keyLength, curKey, sizeof(char) * keyLength);
    }
}

void getFromList(int keyLength, char *keyList, char *key, int pos) {     // fixme: key's memory must be allocated
    memcpy(key, keyList + keyLength * pos, sizeof(char) * keyLength);
}

void setToList(int keyLength, char *keyList, char *key, int pos) {
    if (keyLength == sizeof(PageNum)) {
        PageNum tmp = 0;
        memcpy(&tmp, key, sizeof(PageNum));
        if (tmp == 0) {
            printf("??");
        }
    }

    memcpy(keyList + keyLength * pos, key, sizeof(char) * keyLength);
}

bool checkValid(IX_Node * node, int order) {
    if (node->parent == 0) {
        return node->keynum <= order;
    }
    return node->keynum >= (ceil((double) order / 2.0)) && node->keynum <= order;
}

PageNum findOnesLeftSibling(IX_IndexHandle *indexHandle, PageNum aimPageNode, IX_Node * aimNode) {
    PageNum parentPageNum = aimNode->parent;
    IX_Node * parentNode;
    auto parentPage = new PF_PageHandle;
    char * parentChildren;
    char * parent_src_data;
    int up_step = 0;

    while (parentPageNum != 0) {
        int numAsChild = -1;
        parentNode = getIxNode(indexHandle, parentPageNum, parentPage);
        GetData(parentPage, &parent_src_data);
        parentChildren = parent_src_data + parentNode->rids_offset;
        // initialize parent's info

        for (int i = 0; i < parentNode->keynum + 1; i++) {
            PageNum curChild;
            memcpy(&curChild, parentChildren + i * sizeof(PageNum), sizeof(PageNum));
            if (curChild == aimPageNode) {
                numAsChild = i;
                break;
            }
        }
        UnpinPage(parentPage);
        if (numAsChild == 0) {
            aimPageNode = parentPageNum;
            parentPageNum = parentNode->parent;
            up_step++;
            continue;
        }

        PageNum leftSiblingPageNum;
        memcpy(&leftSiblingPageNum, parentChildren + (numAsChild - 1) * sizeof(PageNum), sizeof(PageNum));
        // find the ancestor

        IX_Node * leftSiblingNode;
        char * leftSiblingChildren;
        char * leftSibling_src_data;

        for (int i = 0; i < up_step; i++) {
            leftSiblingNode = getIxNode(indexHandle, leftSiblingPageNum, parentPage);
            GetData(parentPage, &leftSibling_src_data);
            leftSiblingChildren = leftSibling_src_data + leftSiblingNode->rids_offset;
            memcpy(&leftSiblingPageNum, leftSiblingChildren + leftSiblingNode->keynum * sizeof(PageNum), sizeof(PageNum));
            UnpinPage(parentPage);
        }

        delete parentPage;
        return leftSiblingPageNum;
    }
    delete parentPage;
    return 0;
}

RC InsertEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid) {
    AttrType attrType = indexHandle->fileHeader->attrType;
    int attrLength = indexHandle->fileHeader->attrLength;
    int keyLength = indexHandle->fileHeader->keyLength;

    char realKey[keyLength];
    memcpy(realKey, pData, sizeof(char) * attrLength);
    memcpy(realKey + attrLength, rid, sizeof(RID));
    // basic info

    auto aimNodePageNum = findKey(indexHandle, attrType, attrLength, keyLength, realKey);
    auto aimNodePage = new PF_PageHandle;
    auto aimNode = getIxNode(indexHandle, aimNodePageNum, aimNodePage);

    char *src_data;
    GetData(aimNodePage, &src_data);
    // initialize start info

    char *aimNodeKeyList = src_data + aimNode->keys_offset;
    char *aimNodeRidList = src_data + aimNode->rids_offset;
    int addPos = -1;
    for (int i = 0; i < aimNode->keynum; i++) {
        char *curKey = aimNodeKeyList + sizeof(char) * i * keyLength;
        if (compareKey(curKey, realKey, attrType, attrLength) > 0) {
            addPos = i;
            break;
        }
    }
    if (addPos == -1) {
        addPos = aimNode->keynum;
    } // confirm insert pos

    addToList(keyLength, aimNodeKeyList, aimNode->keynum, realKey, addPos);
    addToList(sizeof(RID), aimNodeRidList, aimNode->keynum, (char *) rid, addPos);
    aimNode->keynum++;
    // for leaf, key.size = children.size
    bool isValid = checkValid(aimNode, indexHandle->fileHeader->order);

    if (isValid) {
        MarkDirty(indexHandle->fileHandle, aimNodePage);
        UnpinPage(aimNodePage);
        delete aimNodePage;
        return SUCCESS;
    }

    PageNum parentPageNum;
    auto parentPage = new PF_PageHandle;
    IX_Node * parentNode;
    char * parentKeyList, * parentChildren;
    char * parent_src_data;

    while (!isValid) {
        int newIdxPos = aimNode->keynum / 2;
        auto newIdx = new char[keyLength];
        getFromList(keyLength, aimNodeKeyList, newIdx, newIdxPos);
        int numAsChild = -1;
        parentPageNum = aimNode->parent;

        bool shouldCreateNew = parentPageNum == 0;
        if (shouldCreateNew) {
            parentPageNum = createNewNode(indexHandle);
        }

        if (aimNodePageNum == 204) {
            printf("???");
        }

        if (parentPageNum == 204) {
            printf("???");
        }

        parentNode = getIxNode(indexHandle, parentPageNum, parentPage);
        GetData(parentPage, &parent_src_data);
        parentKeyList = parent_src_data + parentNode->keys_offset;
        parentChildren = parent_src_data + parentNode->rids_offset;

        if (shouldCreateNew) {
            addToList(sizeof(PageNum), parentChildren, 0, (char *) &aimNodePageNum, 0);
            // do not need to add up on key num
            aimNode->parent = parentPageNum;
            indexHandle->fileHeader->rootPage = parentPageNum;
            MarkDirty(indexHandle->fileHandle, indexHandle->headerPage);
        }   // change the root
        // prepare those data pointer

        for (int i = 0; i < parentNode->keynum; i++) {
            char *curKey = parentKeyList + sizeof(char) * i * keyLength;
            if (compareKey(curKey, realKey, attrType, attrLength) > 0) {
                numAsChild = i;
                break;
            }
        }
        if (numAsChild == -1) {
            numAsChild = parentNode->keynum;
        } // find pos as a child
//
        PageNum leftSiblingPageNum = 0;
        PageNum rightSiblingPageNum = 0;
        if (numAsChild != 0) {
            getFromList(sizeof(PageNum), parentChildren, (char *) &leftSiblingPageNum, numAsChild - 1);
        }
        if (numAsChild != parentNode->keynum) {
            getFromList(sizeof(PageNum), parentChildren, (char *) &rightSiblingPageNum, numAsChild + 1);
        } // find its left and right sibling

        PageNum newLeftChildPageNum = createNewNode(indexHandle);
        auto newLeftChildPage = new PF_PageHandle;
        IX_Node * newLeftChildNode;
        char * newLeftChildKeyList, * newLeftChildChildren;
        char * newLeftChild_src_data;

        newLeftChildNode = getIxNode(indexHandle, newLeftChildPageNum, newLeftChildPage);
        GetData(newLeftChildPage, &newLeftChild_src_data);
        newLeftChildKeyList = newLeftChild_src_data + newLeftChildNode->keys_offset;
        newLeftChildChildren = newLeftChild_src_data + newLeftChildNode->rids_offset;

        newLeftChildNode->is_leaf = aimNode->is_leaf;
        newLeftChildNode->parent = parentPageNum;
        for (int i = 0; i < newIdxPos; i++) {
            char addedKey[keyLength];
            getFromList(keyLength, aimNodeKeyList, addedKey, i);
            addToList(keyLength, newLeftChildKeyList, newLeftChildNode->keynum, addedKey, newLeftChildNode->keynum);
            newLeftChildNode->keynum++;
        }
        if (!aimNode->is_leaf) {
            for (int i = 0; i <= newIdxPos; i++) {
                PageNum addedChildPageNum = 0;
                auto aimChildNodePage = new PF_PageHandle;
                getFromList(sizeof(PageNum), aimNodeRidList, (char *) &addedChildPageNum, i);
                auto addChildNode = getIxNode(indexHandle, addedChildPageNum, aimChildNodePage);
                addChildNode->parent = newLeftChildPageNum;
                addToList(sizeof(PageNum), newLeftChildChildren, i, (char *) &addedChildPageNum, i);

                MarkDirty(indexHandle->fileHandle, aimChildNodePage);
                UnpinPage(aimChildNodePage);
                delete aimChildNodePage;
                // it will stop at the correct num
            }
        } else {
            for (int i = 0; i < newIdxPos; i++) {
                auto addedRid = new RID;
                getFromList(sizeof(RID), aimNodeRidList, (char *) addedRid, i);
                addToList(sizeof(RID), newLeftChildChildren, i, (char *) addedRid, i);
                delete addedRid;
            }
        } // add children

        PageNum newRightChildPageNum = createNewNode(indexHandle);
        auto newRightChildPage = new PF_PageHandle;
        IX_Node * newRightChildNode;
        char * newRightChildKeyList, * newRightChildChildren;
        char * newRightChild_src_data;

        newRightChildNode = getIxNode(indexHandle, newRightChildPageNum, newRightChildPage);
        GetData(newRightChildPage, &newRightChild_src_data);
        newRightChildKeyList = newRightChild_src_data + newRightChildNode->keys_offset;
        newRightChildChildren = newRightChild_src_data + newRightChildNode->rids_offset;

        newRightChildNode->is_leaf = aimNode->is_leaf;
        newRightChildNode->parent = parentPageNum;
        for (int i = newIdxPos + (aimNode->is_leaf ? 0 : 1); i < aimNode->keynum; i++) {
            char addedKey[keyLength];
            getFromList(keyLength, aimNodeKeyList, addedKey, i);
            addToList(keyLength, newRightChildKeyList, newRightChildNode->keynum, addedKey, newRightChildNode->keynum);
            newRightChildNode->keynum++;
        }
        if (!aimNode->is_leaf) {
            for (int i = newIdxPos + 1; i <= aimNode->keynum; i++) {
                PageNum addedChildPageNum = 0;
                auto aimChildNodePage = new PF_PageHandle;
                getFromList(sizeof(PageNum), aimNodeRidList, (char *) &addedChildPageNum, i);
                auto addChildNode = getIxNode(indexHandle, addedChildPageNum, aimChildNodePage);
                addChildNode->parent = newRightChildPageNum;
                addToList(sizeof(PageNum), newRightChildChildren, i - (newIdxPos + 1), (char *) &addedChildPageNum, i - (newIdxPos + 1));

                MarkDirty(indexHandle->fileHandle, aimChildNodePage);
                UnpinPage(aimChildNodePage);
                delete aimChildNodePage;
                // it will stop at the correct num
            }
        } else {
            for (int i = newIdxPos; i < aimNode->keynum; i++) {
                auto addedRid = new RID;
                getFromList(sizeof(RID), aimNodeRidList, (char *) addedRid, i);
                addToList(sizeof(RID), newRightChildChildren, i - (newIdxPos), (char *) addedRid, i - (newIdxPos));

                delete addedRid;
            }
        } // add children

        if (shouldCreateNew) {
            leftSiblingPageNum = 0;
        } else {
            leftSiblingPageNum = findOnesLeftSibling(indexHandle, aimNodePageNum, aimNode);
        }
        rightSiblingPageNum = aimNode->brother;

        if (leftSiblingPageNum != 0) {
            auto leftSiblingPage = new PF_PageHandle;
            auto leftSiblingNode = getIxNode(indexHandle, leftSiblingPageNum, leftSiblingPage);
            leftSiblingNode->brother = newLeftChildPageNum;
            MarkDirty(indexHandle->fileHandle, leftSiblingPage);
            UnpinPage(leftSiblingPage);
            delete leftSiblingPage;
        }
        newLeftChildNode->brother = newRightChildPageNum;
        if (rightSiblingPageNum != 0) {
            auto rightSiblingPage = new PF_PageHandle;
            auto rightSiblingNode = getIxNode(indexHandle, rightSiblingPageNum, rightSiblingPage);
            newRightChildNode->brother = rightSiblingPageNum;
            MarkDirty(indexHandle->fileHandle, rightSiblingPage);
            UnpinPage(rightSiblingPage);
            delete rightSiblingPage;
        }
        // set all siblings

        addToList(keyLength, parentKeyList, parentNode->keynum, newIdx, numAsChild);
        parentNode->keynum++;
        setToList(sizeof(PageNum), parentChildren, (char *) &newLeftChildPageNum, numAsChild);
        addToList(sizeof(PageNum), parentChildren, parentNode->keynum, (char *) &newRightChildPageNum, numAsChild + 1);
        // set children

        DisposePage(indexHandle->fileHandle, aimNodePageNum);
        delete aimNodePage;
        // Destroy formal page

        aimNode = parentNode;
        aimNodeKeyList = parentKeyList;
        aimNodeRidList = parentChildren;
        aimNodePageNum = parentPageNum;
        aimNodePage = parentPage;
        parentPage = new PF_PageHandle;
        // aimNode = parentNode

        MarkDirty(indexHandle->fileHandle, newLeftChildPage);
        MarkDirty(indexHandle->fileHandle, newRightChildPage);
        UnpinPage(newLeftChildPage);
        UnpinPage(newRightChildPage);
        delete newLeftChildPage;
        delete newRightChildPage;
        delete[] newIdx;

        isValid = checkValid(aimNode, indexHandle->fileHeader->order);
    }

    MarkDirty(indexHandle->fileHandle, aimNodePage);
    UnpinPage(aimNodePage);
//    UnpinPage(headerPage);
    delete aimNodePage;
//    delete headerPage;
    delete parentPage;
    return SUCCESS;
}

RC DeleteEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid, int flag) {
    AttrType attrType = indexHandle->fileHeader->attrType;
    int attrLength = indexHandle->fileHeader->attrLength;
    int keyLength = indexHandle->fileHeader->keyLength;
    int order = indexHandle->fileHeader->order;
//    auto headerPage = new PF_PageHandle;
//    GetThisPage(indexHandle->fileHandle, 1, headerPage);

    if (flag == 13) {
        PageNum parentPageNum;
        auto parentPage = new PF_PageHandle;
        IX_Node * parentNode;
        char * parentKeyList, * parentChildren;
        char * parent_src_data;

        parentPageNum = 37;
        parentNode = getIxNode(indexHandle, parentPageNum, parentPage);
        GetData(parentPage, &parent_src_data);
        parentKeyList = parent_src_data + parentNode->keys_offset;
        parentChildren = parent_src_data + parentNode->rids_offset;
        printf("??");
    }

    char realKey[keyLength];
    memset(realKey, 0, (size_t) keyLength);
    memcpy(realKey, pData, sizeof(char) * attrLength);
    memcpy(realKey + attrLength, rid, sizeof(RID));
    // basic info

    int deletePos = -1;
    auto aimNodePageNum = findKey(indexHandle, attrType, attrLength, keyLength, realKey);
    auto aimNodePage = new PF_PageHandle;
    auto aimNode = getIxNode(indexHandle, aimNodePageNum, aimNodePage);
    char *src_data;
    GetData(aimNodePage, &src_data);
    char *aimNodeKeyList = src_data + aimNode->keys_offset;
    char *aimNodeRidList = src_data + aimNode->rids_offset;
    for (int i = 0; i < aimNode->keynum; i++) {
        char *curKey = aimNodeKeyList + sizeof(char) * i * keyLength;
        if (compareKey(curKey, realKey, attrType, attrLength) == 0) {
            deletePos = i;
            break;
        }
    }
    if (deletePos == -1) {
        return IX_INVALIDKEY;
    } // check whether the key exists

    char dumpKey[keyLength], dumpRid[sizeof(RID)], dumpPageNum[sizeof(PageNum)];
    removeFromList(keyLength, aimNodeKeyList, aimNode->keynum, dumpKey, deletePos);
    removeFromList(sizeof(RID), aimNodeRidList, aimNode->keynum, dumpRid, deletePos);
    aimNode->keynum--;
    bool isValid = checkValid(aimNode, order);
    // delete the value

    if (isValid) {
        MarkDirty(indexHandle->fileHandle, aimNodePage);
        UnpinPage(aimNodePage);
//        UnpinPage(headerPage);
//        delete headerPage;
        delete aimNodePage;
        return SUCCESS;
    }

    PageNum parentPageNum;
    auto parentPage = new PF_PageHandle;
    IX_Node * parentNode;
    char * parentKeyList, * parentChildren;
    char * parent_src_data;

    while (!isValid) {
        int numAsChild = -1;
        parentPageNum = aimNode->parent;
        parentNode = getIxNode(indexHandle, parentPageNum, parentPage);
        GetData(parentPage, &parent_src_data);
        parentKeyList = parent_src_data + parentNode->keys_offset;
        parentChildren = parent_src_data + parentNode->rids_offset;
        // initialize parent's info

        for (int i = 0; i < parentNode->keynum; i++) {
            char *curKey = parentKeyList + sizeof(char) * i * keyLength;
            if (compareKey(curKey, realKey, attrType, attrLength) > 0) {
                numAsChild = i;
                break;
            }
        }
        if (numAsChild == -1) {
            numAsChild = parentNode->keynum;
        } // find pos as a child

        PageNum leftSiblingPageNum = 0;
        PageNum rightSiblingPageNum = 0;
        IX_Node * leftSiblingNode = nullptr, * rightSiblingNode = nullptr;
        char * leftSiblingKeyList, * leftSiblingChildList;
        char * rightSiblingKeyList, * rightSiblingChildList;
        char * leftSiblingSrcData, * rightSiblingSrcData;

        bool isLeftEnough = false, isRightEnough = false;
        PF_PageHandle * leftSiblingPage = nullptr;
        PF_PageHandle * rightSiblingPage = nullptr;
        bool hasLeftSibling = false, hasRightSibling = false;
        if (numAsChild != 0) {
            hasLeftSibling = true;
            leftSiblingPage = new PF_PageHandle;
            getFromList(sizeof(PageNum), parentChildren, (char *) &leftSiblingPageNum, numAsChild - 1);
            leftSiblingNode = getIxNode(indexHandle, leftSiblingPageNum, leftSiblingPage);
            isLeftEnough = (leftSiblingNode->keynum > (ceil((double) order / 2.0)));
            GetData(leftSiblingPage, &leftSiblingSrcData);
            leftSiblingKeyList = leftSiblingSrcData + leftSiblingNode->keys_offset;
            leftSiblingChildList = leftSiblingSrcData + leftSiblingNode->rids_offset;
        }
        if (numAsChild != parentNode->keynum) {
            hasRightSibling = true;
            rightSiblingPage = new PF_PageHandle;
            getFromList(sizeof(PageNum), parentChildren, (char *) &rightSiblingPageNum, numAsChild + 1);
            rightSiblingNode = getIxNode(indexHandle, rightSiblingPageNum, rightSiblingPage);
            isRightEnough = (rightSiblingNode->keynum > (ceil((double) order / 2.0)));
            GetData(rightSiblingPage, &rightSiblingSrcData);
            rightSiblingKeyList = rightSiblingSrcData + rightSiblingNode->keys_offset;
            rightSiblingChildList = rightSiblingSrcData + rightSiblingNode->rids_offset;
        } // find its left and right sibling and check whether it's enough

        if (isLeftEnough) {
            if (aimNode->is_leaf) {
                char borrowedKey[keyLength], borrowedRid[sizeof(RID)];
                removeFromList(keyLength, leftSiblingKeyList, leftSiblingNode->keynum, borrowedKey, leftSiblingNode->keynum - 1);
                removeFromList(sizeof(RID), leftSiblingChildList, leftSiblingNode->keynum, borrowedRid, leftSiblingNode->keynum - 1);
                leftSiblingNode->keynum--;
                // remove key

                addToList(keyLength, aimNodeKeyList, aimNode->keynum, borrowedKey, 0);
                addToList(sizeof(RID), aimNodeRidList, aimNode->keynum, borrowedRid, 0);
                aimNode->keynum++;
                // add key

                setToList(keyLength, parentKeyList, borrowedKey, numAsChild - 1);
                // set parent's key
            } else {
                char borrowedKey[keyLength], newParentKey[keyLength];
                char borrowedChildPageNum[sizeof(PageNum)];

                getFromList(keyLength, parentKeyList, borrowedKey, numAsChild - 1);
                removeFromList(keyLength, leftSiblingKeyList, leftSiblingNode->keynum, newParentKey, leftSiblingNode->keynum - 1);
                removeFromList(sizeof(PageNum), leftSiblingChildList, leftSiblingNode->keynum + 1, borrowedChildPageNum, leftSiblingNode->keynum);
                leftSiblingNode->keynum--;

                addToList(keyLength, aimNodeKeyList, aimNode->keynum, borrowedKey, 0);
                addToList(sizeof(PageNum), aimNodeRidList, aimNode->keynum + 1, borrowedChildPageNum, 0);
                aimNode->keynum++;

                setToList(keyLength, parentKeyList, newParentKey, numAsChild - 1);

                PageNum borrowedChildPageNumInt;
                memcpy(&borrowedChildPageNumInt, borrowedChildPageNum, sizeof(PageNum));
                auto borrowedChildPage = new PF_PageHandle;
                auto borrowedChildNode = getIxNode(indexHandle, borrowedChildPageNumInt, borrowedChildPage);
                borrowedChildNode->parent = aimNodePageNum;
                MarkDirty(indexHandle->fileHandle, borrowedChildPage);
                UnpinPage(borrowedChildPage);
                delete borrowedChildPage; // set borrowed child's parent
            }
        } else if (isRightEnough) {
            if (aimNode->is_leaf) {
                char borrowedKey[keyLength], borrowedRid[sizeof(RID)];
                removeFromList(keyLength, rightSiblingKeyList, rightSiblingNode->keynum, borrowedKey, 0);
                removeFromList(sizeof(RID), rightSiblingChildList, rightSiblingNode->keynum, borrowedRid, 0);
                rightSiblingNode->keynum--;
                // remove key

                addToList(keyLength, aimNodeKeyList, aimNode->keynum, borrowedKey, aimNode->keynum);
                addToList(sizeof(RID), aimNodeRidList, aimNode->keynum, borrowedRid, aimNode->keynum);
                aimNode->keynum++;
                // add key

                char newKey[keyLength];
                getFromList(keyLength, rightSiblingKeyList, newKey, 0);
                setToList(keyLength, parentKeyList, newKey, numAsChild);
                // set parent's key
            } else {
                char borrowedKey[keyLength], newParentKey[keyLength];
                char borrowedChildPageNum[sizeof(PageNum)];

                getFromList(keyLength, parentKeyList, borrowedKey, numAsChild);
                removeFromList(keyLength, rightSiblingKeyList, rightSiblingNode->keynum, newParentKey, 0);
                removeFromList(sizeof(PageNum), rightSiblingChildList, rightSiblingNode->keynum + 1, borrowedChildPageNum, 0);
                rightSiblingNode->keynum--;

                addToList(keyLength, aimNodeKeyList, aimNode->keynum, borrowedKey, aimNode->keynum);
                addToList(sizeof(PageNum), aimNodeRidList, aimNode->keynum + 1, borrowedChildPageNum, aimNode->keynum + 1);
                aimNode->keynum++;

                setToList(keyLength, parentKeyList, newParentKey, numAsChild);

                PageNum borrowedChildPageNumInt;
                memcpy(&borrowedChildPageNumInt, borrowedChildPageNum, sizeof(PageNum));
                auto borrowedChildPage = new PF_PageHandle;
                auto borrowedChildNode = getIxNode(indexHandle, borrowedChildPageNumInt, borrowedChildPage);
                borrowedChildNode->parent = aimNodePageNum;
                MarkDirty(indexHandle->fileHandle, borrowedChildPage);
                UnpinPage(borrowedChildPage);
                delete borrowedChildPage; // set borrowed child's parent
            }
        } else {
            if (leftSiblingNode == nullptr) {
                leftSiblingNode = aimNode;
                leftSiblingKeyList = aimNodeKeyList;
                leftSiblingChildList = aimNodeRidList;
                leftSiblingPageNum = aimNodePageNum;
                leftSiblingPage = aimNodePage;
                // leftSibling = aimNode

                aimNode = rightSiblingNode;
                aimNodeKeyList = rightSiblingKeyList;
                aimNodeRidList = rightSiblingChildList;
                aimNodePageNum = rightSiblingPageNum;
                aimNodePage = rightSiblingPage;
                // aimNode = rightSibling

                hasLeftSibling = true;
                hasRightSibling = false;
                numAsChild++;
            } // to ensure aim node always has a left sibling

            if (aimNodePageNum == 3282) {
                PageNum tmpNum;
                auto tmpPage = new PF_PageHandle;
                IX_Node * tmpNode;
                char * tmpKeyList, * tmpChildren;
                char * tmp_src_data;

                tmpNum = 3282;
                tmpNode = getIxNode(indexHandle, tmpNum, tmpPage);
                GetData(tmpPage, &tmp_src_data);
                tmpKeyList = tmp_src_data + tmpNode->keys_offset;
                tmpChildren = tmp_src_data + tmpNode->rids_offset;
                printf("??");
                printf("???");
            }

            if (aimNode == nullptr) {
                return FAIL; // this would never happen
            } else if (aimNode->is_leaf) {
                memcpy(leftSiblingKeyList + keyLength * leftSiblingNode->keynum, aimNodeKeyList, keyLength * aimNode->keynum);
                memcpy(leftSiblingChildList + sizeof(RID) * leftSiblingNode->keynum, aimNodeRidList, sizeof(RID) * aimNode->keynum);
                leftSiblingNode->keynum += aimNode->keynum;
                // addall

                removeFromList(keyLength, parentKeyList, parentNode->keynum, dumpKey, numAsChild - 1);
                removeFromList(sizeof(PageNum), parentChildren, parentNode->keynum + 1, dumpPageNum, numAsChild);
                parentNode->keynum--;

                leftSiblingNode->brother = aimNode->brother;
            } else {
                char downParentKey[keyLength];
                int leftSiblingNodeChildNum = leftSiblingNode->keynum + 1;
                int aimNodeChildNum = aimNode->keynum + 1;

                getFromList(keyLength, parentKeyList, downParentKey, numAsChild - 1);
                addToList(keyLength, leftSiblingKeyList, leftSiblingNode->keynum, downParentKey, leftSiblingNode->keynum);
                leftSiblingNode->keynum++;

                memcpy(leftSiblingKeyList + keyLength * leftSiblingNode->keynum, aimNodeKeyList, keyLength * aimNode->keynum);
                leftSiblingNode->keynum += aimNode->keynum;
                leftSiblingNode->brother = aimNode->brother;

                for (int i = 0; i < aimNodeChildNum; i++) {
                    PageNum curChildPageNum;
                    memcpy(&curChildPageNum, aimNodeRidList + sizeof(PageNum) * i, sizeof(PageNum));

                    auto curChildPage = new PF_PageHandle;
                    auto curChildNode = getIxNode(indexHandle, curChildPageNum, curChildPage);
                    curChildNode->parent = leftSiblingPageNum;

                    MarkDirty(indexHandle->fileHandle, curChildPage);
                    UnpinPage(curChildPage);
                    delete curChildPage;
                }

                memcpy(leftSiblingChildList + sizeof(PageNum) * leftSiblingNodeChildNum, aimNodeRidList, sizeof(PageNum) * (aimNode->keynum + 1));
                // add all children
                removeFromList(sizeof(PageNum), parentChildren, parentNode->keynum + 1, dumpPageNum, numAsChild);
                removeFromList(keyLength, parentKeyList, parentNode->keynum, dumpKey, numAsChild - 1);
                parentNode->keynum--;
            }

//                if (aimNodePageNum == 3144) {
//                    PageNum tmpNum;
//                    auto tmpPage = new PF_PageHandle;
//                    IX_Node * tmpNode;
//                    char * tmpKeyList, * tmpChildren;
//                    char * tmp_src_data;
//
//                    tmpNum = 2795;
//                    tmpNode = getIxNode(indexHandle, tmpNum, tmpPage);
//                    GetData(tmpPage, &tmp_src_data);
//                    tmpKeyList = tmp_src_data + tmpNode->keys_offset;
//                    tmpChildren = tmp_src_data + tmpNode->rids_offset;
//                    printf("??");
//                    printf("???");
//                }
                DisposePage(indexHandle->fileHandle, aimNodePageNum);
        }

        if (hasLeftSibling) {
            MarkDirty(indexHandle->fileHandle, leftSiblingPage);
            UnpinPage(leftSiblingPage);
            delete leftSiblingPage;
        }
        if (hasRightSibling) {
            MarkDirty(indexHandle->fileHandle, rightSiblingPage);
            UnpinPage(rightSiblingPage);
            delete rightSiblingPage;
        }
        MarkDirty(indexHandle->fileHandle, aimNodePage);
        UnpinPage(aimNodePage);
        delete aimNodePage;

        aimNode = parentNode;
        aimNodeKeyList = parentKeyList;
        aimNodeRidList = parentChildren;
        aimNodePageNum = parentPageNum;
        aimNodePage = parentPage;
        parentPage = new PF_PageHandle;
        // aimNode = parentNode
        isValid = checkValid(aimNode, order);
    }

    MarkDirty(indexHandle->fileHandle, aimNodePage);
    UnpinPage(aimNodePage);
    delete aimNodePage;

    auto rootPageHandle = new PF_PageHandle;
    auto rootNode = getIxNode(indexHandle, indexHandle->fileHeader->rootPage, rootPageHandle);
    if (rootNode->keynum == 0) {
        char * rootSrcData;
        GetData(rootPageHandle, &rootSrcData);
        auto rootChild = rootSrcData + rootNode->rids_offset;
        PageNum rootFirstChild = 0;
        memcpy(&rootFirstChild, rootChild, sizeof(PageNum));

        auto rootFirstChildPageHandle = new PF_PageHandle;
        auto rootFirstChildNode = getIxNode(indexHandle, rootFirstChild, rootFirstChildPageHandle);
        rootFirstChildNode->parent = 0;
        rootFirstChildNode->brother = 0;

        MarkDirty(indexHandle->fileHandle, rootFirstChildPageHandle);
        UnpinPage(rootFirstChildPageHandle);
        delete rootFirstChildPageHandle;

        DisposePage(indexHandle->fileHandle, indexHandle->fileHeader->rootPage);
        indexHandle->fileHeader->rootPage = rootFirstChild;
        MarkDirty(indexHandle->fileHandle, indexHandle->headerPage);
    }
    UnpinPage(rootPageHandle);
    delete rootPageHandle;

    delete parentPage;
    return SUCCESS;
}

void traverseAll(IX_IndexHandle *indexHandle) {
    PageNum currentPage = indexHandle->fileHeader->rootPage;
    int keyLength = indexHandle->fileHeader->keyLength;
    int attrLength = indexHandle->fileHeader->attrLength;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    char *src_data;

    while (!currentNode->is_leaf) {
        GetData(currentNodePage, &src_data);
        // Get that data

        char *rids = src_data + currentNode->rids_offset;
        PageNum curChild = * (PageNum *) (rids);
        UnpinPage(currentNodePage);
        currentNode = getIxNode(indexHandle, curChild, currentNodePage);
        currentPage = curChild;
    }

    while (currentPage != 0) {
        GetData(currentNodePage, &src_data);
        // Get that data

        char *keys = src_data + currentNode->keys_offset;
        char *rids = src_data + currentNode->rids_offset;
        for (int i = 0; i < currentNode->keynum; i++) {
            char *curKey = keys + sizeof(char) * i * keyLength;
            char *curRid = rids + sizeof(char) * i * sizeof(RID);

            int prefixKey = 0;
            memcpy(&prefixKey, curKey, sizeof(int));
            RID aftKey;
            memcpy(&aftKey, curKey + attrLength, sizeof(RID));
            RID tmpRid;
            memcpy(&tmpRid, curRid, sizeof(RID));
            printf("PageNum : %d ParentNum : %d Key : %d - %d, %d, Rid : pageNum %d slotNum %d\n", currentPage, currentNode->parent, prefixKey, aftKey.pageNum, aftKey.slotNum, tmpRid.pageNum, tmpRid.slotNum);
        }
        UnpinPage(currentNodePage);
        currentPage = currentNode->brother;
        currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    }

    delete currentNodePage;
}

RC OpenIndexScan(IX_IndexScan *indexScan, IX_IndexHandle *indexHandle, CompOp compOp, char *value) {
    indexScan->bOpen = true;
    indexScan->pIXIndexHandle = indexHandle;
    indexScan->compOp = compOp;
    indexScan->value = value;
    indexScan->pnNext = (PageNum) -1;
    indexScan->ridIx = 0;
    return SUCCESS;
}

RC CloseIndexScan(IX_IndexScan *indexScan) {
    indexScan->bOpen = false;
    indexScan->pIXIndexHandle = nullptr;
    indexScan->value = nullptr;
    indexScan->pnNext = 0;
    indexScan->ridIx = 0;
    return SUCCESS;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan, RID *rid) {
    auto keyLength = indexScan->pIXIndexHandle->fileHeader->keyLength;
    auto attrLength = indexScan->pIXIndexHandle->fileHeader->attrLength;
    auto attrType = indexScan->pIXIndexHandle->fileHeader->attrType;
    char realKey[keyLength];
    bool isNew = false;
    memcpy(realKey, indexScan->value, attrLength);
    RID dumpRid;
    if (indexScan->compOp == GreatT) {
        dumpRid.pageNum = (PageNum) -1;
        dumpRid.pageNum = (unsigned) -1;
    } else {
        dumpRid.pageNum = 0;
        dumpRid.slotNum = 0;
    }
    memcpy(realKey + attrLength, &dumpRid, sizeof(RID));

    if (indexScan->pnNext == -1) {
        isNew = true;
        findStartKey(indexScan, realKey);
    }

    if (indexScan->pnNext == 0) {
        return INDEX_NOT_EXIST; // has checked the last page
    }

    auto aimNodePage = new PF_PageHandle;
    auto aimNode = getIxNode(indexScan->pIXIndexHandle, indexScan->pnNext, aimNodePage);

    if (isNew) {
        if (indexScan->ridIx >= aimNode->keynum) {
            indexScan->ridIx = 0;
            indexScan->pnNext = aimNode->brother;
        }
    }

    char *src_data;
    GetData(aimNodePage, &src_data);
    char *aimNodeKeyList = src_data + aimNode->keys_offset;
    char *aimNodeRidList = src_data + aimNode->rids_offset;
    char *curKey = aimNodeKeyList + sizeof(char) * indexScan->ridIx * keyLength;
    RID tmp;
    memcpy(&tmp, aimNodeRidList + sizeof(RID) * indexScan->ridIx, sizeof(RID));

    if (!checkConditions(indexScan, curKey)) {
        return INDEX_NOT_EXIST;
    }

    rid->bValid = true;
    rid->pageNum = tmp.pageNum;
    rid->slotNum = tmp.slotNum;

    if (indexScan->ridIx + 1 >= aimNode->keynum) {
        indexScan->ridIx = 0;
        indexScan->pnNext = aimNode->brother;
    } else {
        indexScan->ridIx++;
    }
    UnpinPage(aimNodePage);
    return INDEX_EXIST;
}

void generateTreeNode (IX_IndexHandle * indexHandle, PageNum pageNum, Tree_Node * aim_node) {
    int keyLength = indexHandle->fileHeader->keyLength;
    int attrLength = indexHandle->fileHeader->attrLength;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, pageNum, currentNodePage);

    char *src_data;
    GetData(currentNodePage, &src_data);
    char *keys = src_data + currentNode->keys_offset;
    char *rids = src_data + currentNode->rids_offset;

    aim_node->keyNum = currentNode->keynum;
    aim_node->keys = new char * [aim_node->keyNum];
    for (int i = 0; i < aim_node->keyNum; i++) {
        aim_node->keys[i] = new char[keyLength];
        memcpy(aim_node->keys[i], keys + i * keyLength, sizeof(char) * keyLength);
        // cpy all keys
    }
    aim_node->parent = nullptr;
    aim_node->sibling = nullptr;

    if (currentNode->is_leaf) {
        aim_node->firstChild = nullptr;
        return;
    } else {
        aim_node->firstChild = new Tree_Node[aim_node->keyNum + 1];
    }

    for (int i = 0; i < aim_node->keyNum + 1; i++) {
        char *curChild = rids + i * sizeof(PageNum);
        PageNum curChildPageNum = 0;
        memcpy(&curChildPageNum, curChild, sizeof(PageNum));
        if (*curChild == (char) 16) {
            printf("??");
        }
        generateTreeNode(indexHandle, curChildPageNum, &aim_node->firstChild[i]);
        aim_node->firstChild[i].parent = aim_node;
    } // create all children
}

void generateTreeNodeSibling (Tree_Node * aim_node) {
    if (aim_node->firstChild == nullptr) {
        return;
    }

    for (int i = 0; i < aim_node->keyNum + 1; i++) {
        if (i != aim_node->keyNum) {
            aim_node->firstChild[i].sibling = &aim_node->firstChild[i + 1];
        } else {
            if (aim_node->sibling != nullptr) {
                aim_node->firstChild[i].sibling = &aim_node->sibling->firstChild[0];
            } else {
                aim_node->firstChild[i].sibling = nullptr;
            }
        }
    } // create all sibling

    for (int i = 0; i < aim_node->keyNum + 1; i++) {
        generateTreeNodeSibling(&aim_node->firstChild[i]);
    } // create for all children
}

RC GetIndexTree(char *fileName, Tree *index) {
    auto indexHandle = new IX_IndexHandle;
    OpenIndex(fileName, indexHandle);

    PageNum rootPage = indexHandle->fileHeader->rootPage;
    index->attrType = indexHandle->fileHeader->attrType;
    index->attrLength = indexHandle->fileHeader->attrLength;
    index->order = indexHandle->fileHeader->order;
    index->root = new Tree_Node;
    generateTreeNode(indexHandle, rootPage, index->root);
    generateTreeNodeSibling(index->root);

    CloseIndex(indexHandle);
    return SUCCESS;
}
