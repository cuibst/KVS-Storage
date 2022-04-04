#include "indexsystem/indexmanager.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <mutex>

#include "engine.h"

namespace kvs
{

static std::mutex mt;

RID::RID(int ko, int vo) : keyOffset(ko), valueOffset(vo)
{
}

IndexManager::IndexManager(Manager *pfm, const std::string &fileName) : pfm(pfm)
{
    RC rc;
    rc = pfm->CreateFile(fileName.c_str());
    if (rc != 0)
    {
        (rc = pfm->OpenFile(fileName.c_str(), fileHandle));
        assert(rc == 0);
        PageHandle pageHandle;
        char *pageData;
        pinnedPageNum = 0;
        disposedPageNum = 0;
        fileHandle.GetThisPage(0, pageHandle);
        pageHandle.GetData(pageData);
        indexInfo = (IndexInfo *) pageData;
        return;
    }
    rc = pfm->OpenFile(fileName.c_str(), fileHandle);
    assert(rc == 0);
    PageHandle pageHandle;
    fileHandle.AllocatePage(pageHandle);
    char *pageData;
    pageHandle.GetData(pageData);
    PageNum pageNum;
    pageHandle.GetPageNum(pageNum);

    indexInfo = (IndexInfo *) pageData;

    // 0 page is the index info
    indexInfo->rootPageNum = 1;
    fileHandle.MarkDirty(pageNum);
    fileHandle.ForcePages(pageNum);
    fileHandle.UnpinPage(pageNum);

    // 1 page is the initial root page for index
    fileHandle.AllocatePage(pageHandle);
    pageHandle.GetPageNum(pageNum);
    pageHandle.GetData(pageData);
    auto *nodePage = (NodePagePacket *) pageData;
    nodePage->type = NodeType::Leaf;
    nodePage->leafNode.init();
    fileHandle.MarkDirty(pageNum);
    fileHandle.ForcePages(pageNum);
    fileHandle.UnpinPage(pageNum);

    pinnedPageNum = 0;
    disposedPageNum = 0;
    fileHandle.GetThisPage(0, pageHandle);
    pageHandle.GetData(pageData);
    indexInfo = (IndexInfo *) pageData;
}

void IndexManager::insertEntry(const std::string &key,
                               unsigned keyOffset,
                               unsigned valueOffset)
{
    std::ignore = key;  // FIXME: temporarily ignore key.
    RID rid(keyOffset, valueOffset);
    InsertEntryFromPage(key, rid, indexInfo->rootPageNum, -1, -1);
    ForcePages();
    unpinAllPages();
}

void IndexManager::InsertEntryFromPage(const std::string &key,
                                       RID rid,
                                       PageNum &pageNum,
                                       PageNum fatherPage,
                                       int nodePos)
{
    // if (key == "0")
    //     std::cout << "insert " << key << " " << pageNum << std::endl;
    PageHandle pageHandle, fatherPageHandle;
    char *pageData, *fatherPageData;
    if (pageNum == -1)
        pageNum = allocateNewPage(pageHandle);
    else
        getExistedPage(pageNum, pageHandle);

    pageHandle.GetData(pageData);

    NodePagePacket *nodePagePacket = (NodePagePacket *) pageData;

    assert(nodePagePacket->type != ROLeaf &&
           nodePagePacket->type != ROInternal);

    if (nodePagePacket->type == NodeType::Leaf)
    {
        // std::cout << "Type leaf" << std::endl;
        LeafNode *leafNode = &(nodePagePacket->leafNode);
        bool eq = false;
        int L = 0, R = leafNode->size - 1;
        int pos = leafNode->size;
        while (L <= R)
        {
            int mid = (L + R) >> 1;
            int res = cmp(key, (leafNode->data[mid]).keyOffset);
            if (res == 0)
            {
                eq = true;
                pos = mid;
                break;
            }
            if (res < 0)
            {
                pos = mid;
                R = mid - 1;
            }
            else
            {
                L = mid + 1;
            }
        }

        if (eq)
        {
            leafNode->data[pos].keyOffset = rid.keyOffset;
            leafNode->data[pos].valueOffset = rid.valueOffset;
            return;
        }

        leafNode->insertDataIntoPos(rid.keyOffset, rid.valueOffset, pos);
        fileHandle.MarkDirty(pageNum);
        fileHandle.ForcePages(pageNum);

        if (leafNode->size > 2 * D)
        {
            PageHandle splitNodePageHandle;
            PageNum splitNodePageNum = allocateNewPage(splitNodePageHandle);
            fileHandle.MarkDirty(splitNodePageNum);
            fileHandle.MarkDirty(pageNum);
            char *splitNodePageData;
            splitNodePageHandle.GetData(splitNodePageData);
            ((NodePagePacket *) splitNodePageData)->type = NodeType::Leaf;
            LeafNode *splitNode =
                &(((NodePagePacket *) splitNodePageData)->leafNode);
            splitNode->init();
            leafNode->split(splitNode, splitNodePageNum, pageNum);
            // assertIncrease(leafNode);
            // assertIncrease(splitNode);
            fileHandle.ForcePages(pageNum);
            fileHandle.ForcePages(splitNodePageNum);
            if (splitNode->rightPage != -1)
            {
                PageHandle splitRightPageHandle;
                getExistedPage(splitNode->rightPage, splitRightPageHandle);
                char *splitRightPageData;
                splitRightPageHandle.GetData(splitRightPageData);
                LeafNode *splitRightNode =
                    &(((NodePagePacket *) splitRightPageData)->leafNode);
                splitRightNode->leftPage = splitNodePageNum;
                fileHandle.MarkDirty(splitNode->rightPage);
                fileHandle.ForcePages(splitNode->rightPage);
            }
            // change fatherPage
            if (fatherPage == -1)
            {
                fatherPage = allocateNewPage(fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                fileHandle.MarkDirty(fatherPage);
                ((NodePagePacket *) fatherPageData)->type = NodeType::Internal;
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->init();
                fatherNode->child[0] = pageNum;
                fatherNode->offsets[0] = (leafNode->data[0]).keyOffset;
                fatherNode->insertKeyAfterPos(
                    (splitNode->data[0]).keyOffset, splitNodePageNum, 0);
                indexInfo->rootPageNum = fatherPage;
                fileHandle.MarkDirty(0);
                fileHandle.ForcePages(0);
                // assertIncrease(fatherNode);
            }
            else
            {
                getExistedPage(fatherPage, fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->insertKeyAfterPos(
                    (splitNode->data[0]).keyOffset, splitNodePageNum, nodePos);
                fileHandle.MarkDirty(fatherPage);
                // assertIncrease(fatherNode);
            }
            fileHandle.ForcePages(fatherPage);
        }
        else if (fatherPage != -1)
        {
            getExistedPage(fatherPage, fatherPageHandle);
            fatherPageHandle.GetData(fatherPageData);
            InternalNode *fatherNode =
                &(((NodePagePacket *) fatherPageData)->internalNode);
            fatherNode->offsets[nodePos] = (leafNode->data[0]).keyOffset;
            fileHandle.MarkDirty(fatherPage);
            fileHandle.ForcePages(fatherPage);
            // assertIncrease(fatherNode);
        }
    }
    else if (nodePagePacket->type == NodeType::Internal)
    {
        InternalNode *internalNode = &(nodePagePacket->internalNode);
        fileHandle.MarkDirty(pageNum);

        int L = 1, R = internalNode->keyCount;
        int pos = internalNode->keyCount + 1;
        while (L <= R)
        {
            int mid = (L + R) >> 1;
            int res = cmp(key, (internalNode->offsets[mid]));
            if (res < 0)
            {
                pos = mid;
                R = mid - 1;
            }
            else
            {
                L = mid + 1;
            }
        }
        InsertEntryFromPage(
            key, rid, internalNode->child[pos - 1], pageNum, pos - 1);

        // for (int i = 1; i <= internalNode->keyCount + 1; ++i)
        // {
        //     if (i == internalNode->keyCount + 1 ||
        //         cmp(key, internalNode->offsets[i]) < 0)
        //     {
        //         InsertEntryFromPage(
        //             key, rid, internalNode->child[i - 1], pageNum, i - 1);
        //         break;
        //     }
        // }

        if (internalNode->keyCount == D * 2)
        {
            PageHandle splitNodePageHandle;
            PageNum splitNodePageNum = allocateNewPage(splitNodePageHandle);
            fileHandle.MarkDirty(splitNodePageNum);
            fileHandle.MarkDirty(pageNum);
            char *splitNodePageData;
            splitNodePageHandle.GetData(splitNodePageData);
            ((NodePagePacket *) splitNodePageData)->type = NodeType::Internal;
            InternalNode *splitNode =
                &(((NodePagePacket *) splitNodePageData)->internalNode);
            splitNode->init();
            internalNode->split(splitNode);

            fileHandle.ForcePages(pageNum);
            fileHandle.ForcePages(splitNodePageNum);
            // assertIncrease(internalNode);
            // assertIncrease(splitNode);
            //  modify father page
            if (fatherPage == -1)
            {
                fatherPage = allocateNewPage(fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                fileHandle.MarkDirty(fatherPage);
                ((NodePagePacket *) fatherPageData)->type = NodeType::Internal;
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->init();
                fatherNode->child[0] = pageNum;
                fatherNode->offsets[0] = internalNode->offsets[0];
                fatherNode->insertKeyAfterPos(
                    splitNode->offsets[0], splitNodePageNum, 0);
                indexInfo->rootPageNum = fatherPage;
                fileHandle.MarkDirty(0);
                fileHandle.ForcePages(0);
                // assertIncrease(fatherNode);
            }
            else
            {
                getExistedPage(fatherPage, fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->insertKeyAfterPos(
                    splitNode->offsets[0], splitNodePageNum, nodePos);
                fileHandle.MarkDirty(fatherPage);
                // assertIncrease(fatherNode);
            }
            fileHandle.ForcePages(fatherPage);
        }
        else if (fatherPage != -1)
        {
            getExistedPage(fatherPage, fatherPageHandle);
            fatherPageHandle.GetData(fatherPageData);
            InternalNode *fatherNode =
                &(((NodePagePacket *) fatherPageData)->internalNode);
            fatherNode->offsets[nodePos] = internalNode->offsets[0];
            fileHandle.MarkDirty(fatherPage);
            fileHandle.ForcePages(fatherPage);
            // assertIncrease(fatherNode);
        }
    }
}

RC IndexManager::DeleteEntryFromPage(const std::string &key,
                                     RID rid,
                                     PageNum &pageNum,
                                     PageNum fatherPageNum,
                                     int thisPos)
{
    char *pageData, *fatherPageData;
    PageHandle pageHandle, fatherPageHandle;
    getExistedPage(pageNum, pageHandle);
    pageHandle.GetData(pageData);
    // getPageData(pageNum, pageData);
    NodePagePacket *nodePagePacket = (NodePagePacket *) pageData;

    assert(nodePagePacket->type != ROLeaf &&
           nodePagePacket->type != ROInternal);

    if (nodePagePacket->type == NodeType::Internal)
    {
        // internal node, find correct son pos
        InternalNode *internalNode = &(nodePagePacket->internalNode);
        int L = 1, R = internalNode->keyCount;
        int pos = internalNode->keyCount + 1;
        while (L <= R)
        {
            int mid = (L + R) >> 1;
            int res = cmp(key, (internalNode->offsets[mid]));
            if (res < 0)
            {
                pos = mid;
                R = mid - 1;
            }
            else
            {
                L = mid + 1;
            }
        }
        RC rc = DeleteEntryFromPage(
            key, rid, internalNode->child[pos - 1], pageNum, pos - 1);
        if (rc > 0)
            return rc;
        if (internalNode->keyCount == -1)
        {
            // printf("internalNode->keyCount == -1\n");
            addDisposedPage(pageNum);
            if (fatherPageNum != -1)
            {
                getExistedPage(fatherPageNum, fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->deleteKeyAtPos(thisPos);
                fileHandle.MarkDirty(fatherPageNum);
                fileHandle.ForcePages(fatherPageNum);
                // assertIncrease(fatherNode);
            }
        }
        else if (fatherPageNum != -1)
        {
            // update father internalNode head Data
            getExistedPage(fatherPageNum, fatherPageHandle);
            fatherPageHandle.GetData(fatherPageData);
            InternalNode *fatherNode =
                &(((NodePagePacket *) fatherPageData)->internalNode);
            fatherNode->offsets[thisPos] = internalNode->offsets[0];
            fileHandle.MarkDirty(fatherPageNum);
            fileHandle.ForcePages(fatherPageNum);
            // assertIncrease(fatherNode);
        }
        return 0;
    }
    else
    {
        // leaf node
        LeafNode *leafNode = &(nodePagePacket->leafNode);

        bool eq = false;
        int L = 0, R = leafNode->size - 1;
        int pos = leafNode->size;
        while (L <= R)
        {
            int mid = (L + R) >> 1;
            int res = cmp(key, (leafNode->data[mid]).keyOffset);
            if (res == 0)
            {
                eq = true;
                pos = mid;
                break;
            }
            if (res < 0)
            {
                pos = mid;
                R = mid - 1;
            }
            else
            {
                L = mid + 1;
            }
        }

        if (eq)
        {
            // find index val, delete rid
            // reallocate page & delete index val in the tree
            leafNode->deleteDataAtPos(pos);
            fileHandle.MarkDirty(pageNum);
            fileHandle.ForcePages(pageNum);
            // assertIncrease(leafNode);
            if (leafNode->size == 0)
            {
                // printf("leafNode->size == 0\n");
                addDisposedPage(pageNum);
                if (leafNode->leftPage != -1)
                {
                    PageHandle leftPageHandle;
                    getExistedPage(leafNode->leftPage, leftPageHandle);
                    char *leftPageData;
                    leftPageHandle.GetData(leftPageData);
                    LeafNode *leftLeaf =
                        &(((NodePagePacket *) leftPageData)->leafNode);
                    leftLeaf->rightPage = leafNode->rightPage;
                    fileHandle.MarkDirty(leafNode->leftPage);
                    fileHandle.ForcePages(leafNode->leftPage);
                }
                if (leafNode->rightPage != -1)
                {
                    PageHandle rightPageHandle;
                    getExistedPage(leafNode->rightPage, rightPageHandle);
                    char *rightPageData;
                    rightPageHandle.GetData(rightPageData);
                    LeafNode *rightLeaf =
                        &(((NodePagePacket *) rightPageData)->leafNode);
                    rightLeaf->leftPage = leafNode->leftPage;
                    fileHandle.MarkDirty(leafNode->rightPage);
                    fileHandle.ForcePages(leafNode->rightPage);
                }
                if (fatherPageNum != -1)
                {
                    // getPageData(fatherPageNum, fatherPageData);
                    getExistedPage(fatherPageNum, fatherPageHandle);
                    fatherPageHandle.GetData(fatherPageData);
                    InternalNode *fatherNode =
                        &(((NodePagePacket *) fatherPageData)->internalNode);
                    fatherNode->deleteKeyAtPos(thisPos);
                    fileHandle.MarkDirty(fatherPageNum);
                    fileHandle.ForcePages(fatherPageNum);
                    // assertIncrease(fatherNode);
                }
            }
            else if (fatherPageNum != -1)
            {
                // update internalNode head data
                getExistedPage(fatherPageNum, fatherPageHandle);
                fatherPageHandle.GetData(fatherPageData);
                InternalNode *fatherNode =
                    &(((NodePagePacket *) fatherPageData)->internalNode);
                fatherNode->offsets[thisPos] = (leafNode->data[0]).keyOffset;
                fileHandle.MarkDirty(fatherPageNum);
                fileHandle.ForcePages(fatherPageNum);
                // assertIncrease(fatherNode);
            }
            return 0;
        }
        return 1;
    }
}

bool IndexManager::deleteEntry(const std::string &key, unsigned keyOffset)
{
    RC retCode = DeleteEntryFromPage(
        key, RID(keyOffset, -1), indexInfo->rootPageNum, -1, -1);
    ForcePages();
    unpinAllPages();
    disposeAllPages();
    if (retCode != 0)
        return false;
    return true;
}

IndexManager &IndexManager::getInstance(const std::string &indexFileName)
{
    // std::cout << "logging" << std::endl;
    static Manager manager;
    // std::cout << "logging" << std::endl;
    static IndexManager im(&manager, indexFileName);
    // std::cout << "logging" << std::endl;
    return im;
}

// int IndexManager::cmp(const unsigned &offsetA, const unsigned &offsetB)
// {
//     std::string keyA, keyB;
//     keyA = Engine::loadKey(offsetA);
//     keyB = Engine::loadKey(offsetB);
//     int l = std::min(keyA.length(), keyB.length());
//     for (int i = 0; i < l; i++)
//         if (keyA[i] != keyB[i])
//             return keyA[i] < keyB[i] ? -1 : 1;
//     if (keyA.length() != keyB.length())
//         return keyA.length() < keyB.length() ? -1 : 1;
//     return 0;
// }

int IndexManager::cmp(const std::string &keyA, const unsigned &offset)
{
    std::string keyB;
    keyB = Engine::loadKey(offset);
    // if (keyA == "0")
    //     std::cout << "compare 0 with " << keyB << std::endl;
    int l = std::min(keyA.length(), keyB.length());
    for (int i = 0; i < l; i++)
        if (keyA[i] != keyB[i])
            return keyA[i] < keyB[i] ? -1 : 1;
    if (keyA.length() != keyB.length())
        return keyA.length() < keyB.length() ? -1 : 1;
    return 0;
}

IndexManager::~IndexManager()
{
    pfm->CloseFile(fileHandle);
}

int IndexManager::getLeafNodeSize(const PageNum pageNum)
{
    char *pageData;
    getPageData(pageNum, pageData);
    return ((NodePagePacket *) pageData)->leafNode.size;
}

void IndexManager::getPageData(const PageNum pageNum, char *&pageData)
{
    PageHandle pageHandle;
    assert(pageNum != -1);
    getExistedPage(pageNum, pageHandle);
    pageHandle.GetData(pageData);
}

void IndexManager::getLeafNode(const PageNum pageNum, LeafNode *&leafNode)
{
    char *pageData;
    getPageData(pageNum, pageData);
    leafNode = &(((NodePagePacket *) pageData)->leafNode);
}

void IndexManager::getInternalNode(const PageNum pageNum,
                                   InternalNode *&internalNode)
{
    char *pageData;
    getPageData(pageNum, pageData);
    internalNode = &(((NodePagePacket *) pageData)->internalNode);
}

PageNum IndexManager::FindLeafPageFromPage(const std::string &key,
                                           PageNum pageNum)
{
    // std::cout << "tree find " << key << " " << pageNum << std::endl;
    PageNum result = -1;
    PageHandle pageHandle;
    fileHandle.GetThisPage(pageNum, pageHandle);
    char *pageData;
    pageHandle.GetData(pageData);
    NodePagePacket *nodePagePacket = (NodePagePacket *) pageData;
    if (nodePagePacket->type == NodeType::Leaf ||
        nodePagePacket->type == NodeType::ROLeaf)
    {  // leafNode
        result = pageNum;
    }
    else
    {  // internalNode
        InternalNode *internalNode = &(nodePagePacket->internalNode);
        for (int i = 1; i <= internalNode->keyCount + 1; ++i)
        {
            if (cmp(key, internalNode->offsets[i]) < 0)
            {
                result = FindLeafPageFromPage(key, internalNode->child[i - 1]);
                break;
            }
            else if (i == internalNode->keyCount + 1)
            {
                result = FindLeafPageFromPage(
                    key, internalNode->child[internalNode->keyCount]);
            }
        }
    }
    fileHandle.UnpinPage(pageNum);
    return result;
}

LeafNode IndexManager::FindLeafNode(const std::string &key, PageNum root)
{
    PageNum leafPage =
        FindLeafPageFromPage(key, root == -1 ? indexInfo->rootPageNum : root);
    // std::cout << "find done with " << leafPage << std::endl;
    PageHandle pageHandle;
    fileHandle.GetThisPage(leafPage, pageHandle);
    char *pageData;
    pageHandle.GetData(pageData);
    fileHandle.UnpinPage(leafPage);
    return ((NodePagePacket *) pageData)->leafNode;
}

bool IndexManager::getEntry(const std::string &key,
                            unsigned &valueOffset,
                            PageNum root)
{
    LeafNode leafNode = FindLeafNode(key, root);

    bool eq = false;
    int L = 0, R = leafNode.size - 1;
    int pos = leafNode.size;
    while (L <= R)
    {
        int mid = (L + R) >> 1;
        int res = cmp(key, (leafNode.data[mid]).keyOffset);
        if (res == 0)
        {
            eq = true;
            pos = mid;
            break;
        }
        if (res < 0)
            R = mid - 1;
        else
            L = mid + 1;
    }
    if (eq)
    {
        valueOffset = leafNode.data[pos].valueOffset;
        return true;
    }
    // std::cout << "tree get " << key << ", size = " << node.size << std::endl;
    // for (int i = 0; i < node.size; i++)
    //     if (cmp(key, (node.data[i]).keyOffset) == 0)
    //     {
    //         valueOffset = node.data[i].valueOffset;
    //         return true;
    //     }
    return false;
}

void IndexManager::visit(
    const std::string &lower,
    const std::string &upper,
    const std::function<void(const std::string &, const std::string &)>
        &visitor,
    PageNum root)
{
    PageNum leafPage =
        FindLeafPageFromPage(lower, root == -1 ? indexInfo->rootPageNum : root);
    bool flag = false;
    while (leafPage != -1)
    {
        LeafNode *leaf;
        getLeafNode(leafPage, leaf);
        for (int i = 0; i < leaf->size; i++)
        {
            std::string tmp = Engine::loadKey(leaf->data[i].keyOffset);
            if (!flag)
                if (tmp == "" || lower <= tmp)
                    flag = true;
            if (flag)
            {
                if (tmp != "" && upper < tmp)
                    goto end;
                std::string value =
                    Engine::loadValue(leaf->data[i].valueOffset);
                visitor(tmp, value);
            }
        }
        leafPage = leaf->rightPage;
    }
end:;
    unpinAllPages();
}

PageNum IndexManager::allocateNewPage(PageHandle &pageHandle)
{
    fileHandle.AllocatePage(pageHandle);
    PageNum pageNum;
    pageHandle.GetPageNum(pageNum);
    addPinnedPage(pageNum);
    return pageNum;
}

void IndexManager::getExistedPage(PageNum pageNum, PageHandle &pageHandle)
{
    fileHandle.GetThisPage(pageNum, pageHandle);
    addPinnedPage(pageNum);
}

void IndexManager::addPinnedPage(PageNum pageNum)
{
    pinnedPageList[pinnedPageNum++] = pageNum;
}

void IndexManager::addDisposedPage(PageNum pageNum)
{
    disposedPageList[disposedPageNum++] = pageNum;
}

void IndexManager::unpinAllPages()
{
    // printf("pinnedPageNum = %d\n", pinnedPageNum);
    for (int i = 0; i < pinnedPageNum; ++i)
    {
        fileHandle.UnpinPage(pinnedPageList[i]);
    }
    pinnedPageNum = 0;
}

void IndexManager::disposeAllPages()
{
    bool rebuild = false;
    for (int i = 0; i < disposedPageNum; ++i)
    {
        if (disposedPageList[i] == indexInfo->rootPageNum)
            rebuild = true;
        fileHandle.DisposePage(disposedPageList[i]);
    }

    disposedPageNum = 0;

    if (rebuild)
    {
        PageHandle pageHandle;
        PageNum pageNum;
        char *pageData;
        fileHandle.AllocatePage(pageHandle);
        pageHandle.GetPageNum(pageNum);
        pageHandle.GetData(pageData);
        NodePagePacket *nodePagePacket = (NodePagePacket *) pageData;
        nodePagePacket->type = NodeType::Leaf;
        (nodePagePacket->leafNode).init();
        fileHandle.MarkDirty(pageNum);
        fileHandle.ForcePages(pageNum);
        fileHandle.UnpinPage(pageNum);
        indexInfo->rootPageNum = pageNum;
        fileHandle.MarkDirty(0);
        fileHandle.ForcePages(0);
    }
}

RC IndexManager::ForcePages()
{
    fileHandle.ForcePages();
    return 0;
}

PageNum IndexManager::getRootPageNum()
{
    return indexInfo->rootPageNum;
}

void IndexManager::persistentAllNode(PageNum pageNum,
                                     std::map<PageNum, PageNum> &pageMapping)
{
    PageHandle pageHandle;
    char *pageData;
    if (pageNum == -1)
        pageNum = allocateNewPage(pageHandle);
    else
        getExistedPage(pageNum, pageHandle);

    pageHandle.GetData(pageData);

    NodePagePacket *nodePagePacket = (NodePagePacket *) pageData;

    if (nodePagePacket->type == NodeType::Leaf)
    {
        PageHandle copyPageHandle;
        char *copyPageData;
        PageNum copyPageNum = allocateNewPage(copyPageHandle);
        copyPageHandle.GetData(copyPageData);
        memcpy(copyPageData, pageData, sizeof(NodePagePacket));
        LeafNode &leaf = nodePagePacket->leafNode;
        LeafNode &newLeaf = ((NodePagePacket *) copyPageData)->leafNode;
        if (leaf.leftPage != -1)
        {
            PageNum mappedLeftPage = pageMapping[leaf.leftPage];
            PageHandle mappedLeftHandle;
            getExistedPage(mappedLeftPage, mappedLeftHandle);
            char *mappedLeftPageData;
            mappedLeftHandle.GetData(mappedLeftPageData);
            LeafNode &mappedLeftLeaf =
                ((NodePagePacket *) mappedLeftPageData)->leafNode;
            mappedLeftLeaf.rightPage = copyPageNum;
            newLeaf.leftPage = mappedLeftPage;
            fileHandle.MarkDirty(mappedLeftPage);
            fileHandle.ForcePages(mappedLeftPage);
        }
        nodePagePacket->type = NodeType::ROLeaf;
        fileHandle.MarkDirty(pageNum);
        fileHandle.ForcePages(pageNum);
        fileHandle.MarkDirty(copyPageNum);
        fileHandle.ForcePages(copyPageNum);
        pageMapping[pageNum] = copyPageNum;
    }
    else if (nodePagePacket->type == NodeType::Internal)
    {
        InternalNode &internalNode = nodePagePacket->internalNode;
        PageHandle copyPageHandle;
        char *copyPageData;
        PageNum copyPageNum = allocateNewPage(copyPageHandle);
        copyPageHandle.GetData(copyPageData);
        memcpy(copyPageData, pageData, sizeof(NodePagePacket));
        InternalNode &newInternal =
            ((NodePagePacket *) copyPageData)->internalNode;
        for (int i = 1; i <= internalNode.keyCount + 1; ++i)
            persistentAllNode(internalNode.child[i - 1], pageMapping);
        for (int i = 1; i <= newInternal.keyCount + 1; i++)
            newInternal.child[i - 1] = pageMapping[newInternal.child[i - 1]];
        nodePagePacket->type = NodeType::ROInternal;
        fileHandle.MarkDirty(pageNum);
        fileHandle.ForcePages(pageNum);
        pageMapping[pageNum] = copyPageNum;
        fileHandle.MarkDirty(copyPageNum);
        fileHandle.ForcePages(copyPageNum);
    }
}

PageNum IndexManager::persistent()
{
    std::map<PageNum, PageNum> tmp;
    persistentAllNode(indexInfo->rootPageNum, tmp);
    int res = indexInfo->rootPageNum;
    indexInfo->rootPageNum = tmp[indexInfo->rootPageNum];
    ForcePages();
    unpinAllPages();

    // PageNum leaf1 = FindLeafPageFromPage("", indexInfo->rootPageNum);
    // PageNum leaf2 = FindLeafPageFromPage("", res);
    // while (leaf1 != -1 && leaf2 != -1)
    // {
    //     LeafNode *leaf11;
    //     getLeafNode(leaf1, leaf11);
    //     LeafNode *leaf22;
    //     getLeafNode(leaf2, leaf22);
    //     assert(tmp[leaf2] == leaf1);
    //     assert(leaf11->size == leaf22->size);
    //     for (int i = 0; i < leaf11->size; i++)
    //     {
    //         std::string tmp1 = Engine::loadKey(leaf11->data[i].keyOffset);
    //         std::string tmp2 = Engine::loadKey(leaf22->data[i].keyOffset);
    //         std::cout << tmp1 << " " << tmp2 << std::endl;
    //         assert(tmp1 == tmp2);
    //     }
    //     leaf1 = leaf11->rightPage;
    //     leaf2 = leaf22->rightPage;
    // }

    // assert(leaf1 == -1);
    // assert(leaf2 == -1);

    // ForcePages();
    // unpinAllPages();

    return res;
}

int IndexManager::rebuildLog(const std::string &path)
{
    std::string newLogPath = path + "/" + "kvslog_cpy.log";
    int newLogFileDescriptor = open(newLogPath.c_str(), O_RDWR);

    PageNum leaf1 = FindLeafPageFromPage("", indexInfo->rootPageNum);
    int offset = 0;
    while (leaf1 != -1)
    {
        LeafNode *leaf11;
        getLeafNode(leaf1, leaf11);
        for (int i = 0; i < leaf11->size; i++)
        {
            std::string key = Engine::loadKey(leaf11->data[i].keyOffset);
            std::string value = Engine::loadValue(leaf11->data[i].valueOffset);
            unsigned char *tmp =
                new unsigned char[key.length() + value.length() + 2];
            // std::cout << key.length() << " " << value.length() << std::endl;
            for (unsigned long i = 0; i < key.length(); i++)
                tmp[i] = key[i];
            tmp[key.length()] = 254;
            for (unsigned long i = 0; i < value.length(); i++)
                tmp[key.length() + i + 1] = value[i];
            tmp[key.length() + value.length() + 1] = 255;
            ssize_t ret = write(
                newLogFileDescriptor, tmp, key.length() + value.length() + 2);
            std::ignore = ret;
            leaf11->data[i].keyOffset = offset;
            leaf11->data[i].valueOffset = offset + key.length() + 1;
            offset += key.length() + value.length() + 2;
            delete[] tmp;
        }
        fileHandle.MarkDirty(leaf1);
        fileHandle.ForcePages(leaf1);
        leaf1 = leaf11->rightPage;
    }
    close(newLogFileDescriptor);

    ForcePages();
    unpinAllPages();
    return offset;
}

}  // namespace kvs