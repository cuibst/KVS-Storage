#pragma once

#include "hashtable.h"
#include "internal.h"

#define INVALID_SLOT (-1)

struct BufPageDesc
{
    char *pData;
    int next;
    int prev;
    int bDirty;
    short int pinCount;
    PageNum pageNum;
    int fd;
};

class BufferManager
{
public:
    BufferManager(int numPages);
    ~BufferManager();

    RC GetPage(int fd, PageNum pageNum, char **ppBuffer, int bMultiplePins = 1);
    RC AllocatePage(int fd, PageNum pageNum, char **ppBuffer);

    RC MarkDirty(int fd, PageNum pageNum);
    RC UnpinPage(int fd, PageNum pageNum);
    RC FlushPages(int fd);
    RC ForcePages(int fd, PageNum pageNum);

private:
    RC InsertFree(int slot);
    RC LinkHead(int slot);
    RC Unlink(int slot);
    RC InternalAlloc(int &slot);

    RC ReadPage(int fd, PageNum pageNum, char *dest);
    RC WritePage(int fd, PageNum pageNum, char *source);
    RC InitPageDesc(int fd, PageNum pageNum, int slot);

    BufPageDesc *bufTable;
    HashTable hashTable;
    int numPages;
    int pageSize;
    int first;
    int last;
    int free;
};
