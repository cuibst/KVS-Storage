#pragma once

#include "internal.h"

struct HashEntry
{
    HashEntry *next;
    HashEntry *prev;
    int fd;
    PageNum pageNum;
    int slot;
};

class HashTable
{
public:
    HashTable(int numBuckets);
    ~HashTable();
    RC Find(int fd, PageNum pageNum, int &slot);
    RC Insert(int fd, PageNum pageNum, int slot);
    RC Delete(int fd, PageNum pageNum);

private:
    int Hash(int fd, PageNum pageNum) const
    {
        return ((fd + pageNum) % numBuckets);
    }
    int numBuckets;
    HashEntry **hashTable;
};
