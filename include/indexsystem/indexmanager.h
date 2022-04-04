#pragma once

#include <functional>
#include <map>

#include "../filesystem/manager.h"
#include "treenode.h"
#define MAX_DEPTH 1010

namespace kvs
{

struct IndexInfo
{
    PageNum rootPageNum;
};

struct RID
{
    int keyOffset;
    int valueOffset;
    RID(int ko, int vo);
};

class IndexManager
{
private:
    IndexManager(Manager *pfm, const std::string &fileName);
    std::map<std::string, unsigned> indices;

    FileHandle fileHandle;
    Manager *pfm;
    IndexInfo *indexInfo;
    int attrOffset;
    PageNum pinnedPageList[MAX_DEPTH];
    int pinnedPageNum;
    PageNum disposedPageList[MAX_DEPTH];
    int disposedPageNum;

    // PageHandle inMemory[100];
    // bool mark[100];

    void InsertEntryFromPage(const std::string &key,
                             RID rid,
                             PageNum &pageNum,
                             PageNum fatherPage,
                             int nodePos);
    RC DeleteEntryFromPage(const std::string &key,
                           RID rid,
                           PageNum &pageNum,
                           PageNum fatherPageNum,
                           int thisPos);
    PageNum FindLeafPageFromPage(const std::string &data, PageNum pageNum);

    int getLeafNodeSize(const PageNum pageNum);
    void getPageData(const PageNum pageNum, char *&pageData);
    void getLeafNode(const PageNum pageNum, LeafNode *&leafNode);
    void getInternalNode(const PageNum pageNum, InternalNode *&internalNode);
    PageNum allocateNewPage(PageHandle &pageHandle);
    void getExistedPage(PageNum pageNum, PageHandle &pageHandle);

    void addPinnedPage(const PageNum pageNum);
    void addDisposedPage(const PageNum pageNum);
    void unpinAllPages();
    void disposeAllPages();
    void persistentAllNode(PageNum now,
                           std::map<PageNum, PageNum> &pageMapping);

public:
    static IndexManager &getInstance(const std::string &indexFileName);

    void insertEntry(const std::string &key,
                     unsigned keyOffset,
                     unsigned valueOffset);
    bool deleteEntry(const std::string &key, unsigned keyOffset);
    bool getEntry(const std::string &key,
                  unsigned &valueOffset,
                  PageNum root = -1);
    void visit(const std::string &lower,
               const std::string &upper,
               const std::function<void(const std::string &,
                                        const std::string &)> &visitor,
               PageNum root = -1);

    // static int cmp(const unsigned &offsetA, const unsigned &offsetB);
    static int cmp(const std::string &key, const unsigned &offset);
    RC ForcePages();
    LeafNode FindLeafNode(const std::string &key, PageNum root = -1);
    ~IndexManager();
    PageNum getRootPageNum();
    PageNum persistent();
    int rebuildLog(const std::string &path);
};
}  // namespace kvs