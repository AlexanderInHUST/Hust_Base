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
    ix_fileHeader->rootPage = 1;
    ix_fileHeader->first_leaf = 1;
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
    memcpy(dst_data + sizeof(IX_FileHeader), ix_node, sizeof(IX_Node));

    MarkDirty(pf_pageHandle);
    UnpinPage(pf_pageHandle);
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

    auto pf_pageHandle = new PF_PageHandle;
    GetThisPage(pf_fileHandle, 1, pf_pageHandle);
    char *src_data;
    GetData(pf_pageHandle, &src_data);

    indexHandle->bOpen = true;
    indexHandle->fileHandle =  pf_fileHandle;
    indexHandle->fileHeader = (IX_FileHeader *) src_data;
    delete pf_pageHandle;
    return SUCCESS;
}

RC CloseIndex(IX_IndexHandle *indexHandle) {
    CloseFile(indexHandle->fileHandle);
    indexHandle->bOpen = false;
    delete indexHandle->fileHandle;
    return SUCCESS;
}

PageNum createNewNode(IX_IndexHandle *indexHandle) {
    auto pf_fileHandle = indexHandle->fileHandle;
    auto pf_pageHandle = new PF_PageHandle();
    pf_pageHandle->bOpen = true;
    AllocatePage(pf_fileHandle, pf_pageHandle);

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

    // debug:
    printf("Create Page : %d\n", newPageNum);

    delete pf_pageHandle;
    delete ix_node;
    return newPageNum;
}

IX_Node * getIxNode(IX_IndexHandle *indexHandle, PageNum pageNum, PF_PageHandle * pf_pageHandle) {
    GetThisPage(indexHandle->fileHandle, pageNum, pf_pageHandle);
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
        return rid1->pageNum > rid2->pageNum ? 1 : -1;
    } else {
        return rid1->slotNum > rid2->slotNum ? 1 : -1;
    }
}

PageNum findKey(IX_IndexHandle *indexHandle, AttrType attrType, int attrLength, int keyLength, char *key) {
    PageNum currentPage = indexHandle->fileHeader->rootPage;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    char *src_data;

    while (!currentNode->is_leaf) {
        bool isFound = false;
        GetThisPage(indexHandle->fileHandle, currentPage, currentNodePage);
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

void addToList(int keyLength, char *keyList, int list_size, char *key, int pos) {
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
    memcpy(keyList + keyLength * pos, key, sizeof(char) * keyLength);
}

bool checkValid(IX_Node * node, int order) {
    if (node->parent == 0) {
        return node->keynum <= order;
    }
    return node->keynum >= (ceil((double) order / 2.0)) && node->keynum <= order;
}

RC InsertEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid) {
    AttrType attrType = indexHandle->fileHeader->attrType;
    int attrLength = indexHandle->fileHeader->attrLength;
    int keyLength = indexHandle->fileHeader->keyLength;
    auto headerPage = new PF_PageHandle;
    GetThisPage(indexHandle->fileHandle, 1, headerPage);
    // basic info

    auto aimNodePageNum = findKey(indexHandle, attrType, attrLength, keyLength, pData);
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
        if (compareKey(curKey, pData, attrType, attrLength) > 0) {
            addPos = i;
            break;
        }
    }
    if (addPos == -1) {
        addPos = aimNode->keynum;
    } // confirm insert pos

    addToList(keyLength, aimNodeKeyList, aimNode->keynum, pData, addPos);
    addToList(sizeof(RID), aimNodeRidList, aimNode->keynum, (char *) rid, addPos);
    aimNode->keynum++;
    // for leaf, key.size = children.size
    bool isValid = checkValid(aimNode, indexHandle->fileHeader->order);

    if (isValid) {
        MarkDirty(aimNodePage);
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

        parentNode = getIxNode(indexHandle, parentPageNum, parentPage);
        GetData(parentPage, &parent_src_data);
        parentKeyList = parent_src_data + parentNode->keys_offset;
        parentChildren = parent_src_data + parentNode->rids_offset;

        if (shouldCreateNew) {
            addToList(sizeof(PageNum), parentChildren, 0, (char *) &aimNodePageNum, 0);
            // do not need to add up on key num
            aimNode->parent = parentPageNum;
            indexHandle->fileHeader->rootPage = parentPageNum;
            MarkDirty(headerPage);
        }   // change the root
        // prepare those data pointer

        for (int i = 0; i < parentNode->keynum; i++) {
            char *curKey = parentKeyList + sizeof(char) * i * keyLength;
            if (compareKey(curKey, pData, attrType, attrLength) > 0) {
                numAsChild = i;
                break;
            }
        }
        if (numAsChild == -1) {
            numAsChild = parentNode->keynum;
        } // find pos as a child

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

                MarkDirty(aimChildNodePage);
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

                MarkDirty(aimChildNodePage);
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

        auto leftSiblingPage = new PF_PageHandle;
        auto rightSiblingPage = new PF_PageHandle;
        auto leftSiblingNode = getIxNode(indexHandle, leftSiblingPageNum, leftSiblingPage);
        auto rightSiblingNode = getIxNode(indexHandle, rightSiblingPageNum, rightSiblingPage);
        if (leftSiblingPageNum != 0) {
            leftSiblingNode->brother = newLeftChildPageNum;
        }
        newLeftChildNode->brother = newRightChildPageNum;
        if (rightSiblingPageNum != 0) {
            newRightChildNode->brother = rightSiblingPageNum;
        }
        MarkDirty(leftSiblingPage);
        MarkDirty(rightSiblingPage);
        UnpinPage(leftSiblingPage);
        UnpinPage(rightSiblingPage);
        delete leftSiblingPage;
        delete rightSiblingPage;
        // set all siblings

        addToList(keyLength, parentKeyList, parentNode->keynum, newIdx, numAsChild);
        parentNode->keynum++;
        setToList(sizeof(PageNum), parentChildren, (char *) &newLeftChildPageNum, numAsChild);
        addToList(sizeof(PageNum), parentChildren, parentNode->keynum, (char *) &newRightChildPageNum, numAsChild + 1);
        // set children

        if (aimNodePageNum != 1) {
            DisposePage(indexHandle->fileHandle, aimNodePageNum);
            // debug:
            printf("Dispose Page : %d\n", aimNodePageNum);
        }
        delete aimNodePage;
        // Destroy formal page

        aimNode = parentNode;
        aimNodeKeyList = parentKeyList;
        aimNodeRidList = parentChildren;
        aimNodePageNum = parentPageNum;
        aimNodePage = parentPage;
        parentPage = new PF_PageHandle;
        // aimNode = parentNode

        MarkDirty(newLeftChildPage);
        MarkDirty(newRightChildPage);
        UnpinPage(newLeftChildPage);
        UnpinPage(newRightChildPage);
        delete newLeftChildPage;
        delete newRightChildPage;
        delete[] newIdx;

        isValid = checkValid(aimNode, indexHandle->fileHeader->order);
    }

    MarkDirty(parentPage);
    UnpinPage(parentPage);
    UnpinPage(headerPage);
    delete headerPage;
    delete parentPage;
    return SUCCESS;
}

RC DeleteEntry(IX_IndexHandle *indexHandle, char *pData, const RID *rid) {
    return PF_NOBUF;
}

void traverseAll(IX_IndexHandle *indexHandle) {
    PageNum currentPage = indexHandle->fileHeader->rootPage;
    int keyLength = indexHandle->fileHeader->keyLength;
    int attrType = indexHandle->fileHeader->attrLength;
    auto currentNodePage = new PF_PageHandle;
    auto currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    char *src_data;

    while (!currentNode->is_leaf) {
        GetThisPage(indexHandle->fileHandle, currentPage, currentNodePage);
        GetData(currentNodePage, &src_data);
        // Get that data

        char *rids = src_data + currentNode->rids_offset;
        PageNum curChild = * (PageNum *) (rids);
        UnpinPage(currentNodePage);
        currentNode = getIxNode(indexHandle, curChild, currentNodePage);
        currentPage = curChild;
    }

    while (currentPage != 0) {
        GetThisPage(indexHandle->fileHandle, currentPage, currentNodePage);
        GetData(currentNodePage, &src_data);
        // Get that data

        char *keys = src_data + currentNode->keys_offset;
        char *rids = src_data + currentNode->rids_offset;
        for (int i = 0; i < currentNode->keynum; i++) {
            char *curKey = keys + sizeof(char) * i * keyLength;
            char *curRid = rids + sizeof(char) * i * sizeof(RID);

            int prefixKey = 0;
            memcpy(&prefixKey, curKey, sizeof(int));
            RID tmpRid;
            memcpy(&tmpRid, curRid, sizeof(RID));
            printf("Key : %d, Rid : pageNum %d slotNum %d\n", prefixKey, tmpRid.pageNum, tmpRid.slotNum);
        }

        currentPage = currentNode->brother;
        currentNode = getIxNode(indexHandle, currentPage, currentNodePage);
    }

    UnpinPage(currentNodePage);
    delete currentNodePage;
}

RC OpenIndexScan(IX_IndexScan *indexScan, IX_IndexHandle *indexHandle, CompOp compOp, char *value) {
    return PF_NOBUF;
}

RC IX_GetNextEntry(IX_IndexScan *indexScan, RID *rid) {
    return PF_NOBUF;
}

RC CloseIndexScan(IX_IndexScan *indexScan) {
    return PF_NOBUF;
}

void generateTreeNode (IX_IndexHandle * indexHandle, PageNum pageNum, Tree_Node * aim_node) {
    int keyLength = indexHandle->fileHeader->keyLength;
    int attrType = indexHandle->fileHeader->attrLength;
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
    return SUCCESS;
}
