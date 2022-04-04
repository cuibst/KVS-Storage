#include "filesystem/hashtable.h"

#include "filesystem/internal.h"

HashTable::HashTable(int _numBuckets)
{
    this->numBuckets = _numBuckets;

    hashTable = new HashEntry *[numBuckets];

    for (int i = 0; i < numBuckets; i++)
        hashTable[i] = NULL;
}

HashTable::~HashTable()
{
    for (int i = 0; i < numBuckets; i++)
    {
        HashEntry *entry = hashTable[i];
        while (entry != NULL)
        {
            HashEntry *next = entry->next;
            delete entry;
            entry = next;
        }
    }

    delete[] hashTable;
}

RC HashTable::Find(int fd, PageNum pageNum, int &slot)
{
    int bucket = Hash(fd, pageNum);

    if (bucket < 0)
        return -1;
    for (HashEntry *entry = hashTable[bucket]; entry != NULL;
         entry = entry->next)
        if (entry->fd == fd && entry->pageNum == pageNum)
        {
            slot = entry->slot;
            return 0;
        }
    return -1;
}

RC HashTable::Insert(int fd, PageNum pageNum, int slot)
{
    int bucket = Hash(fd, pageNum);
    HashEntry *entry;
    for (entry = hashTable[bucket]; entry != NULL; entry = entry->next)
    {
        if (entry->fd == fd && entry->pageNum == pageNum)
            return -1;
    }
    if ((entry = new HashEntry) == NULL)
        return -1;
    entry->fd = fd;
    entry->pageNum = pageNum;
    entry->slot = slot;
    entry->next = hashTable[bucket];
    entry->prev = NULL;
    if (hashTable[bucket] != NULL)
        hashTable[bucket]->prev = entry;
    hashTable[bucket] = entry;
    return 0;
}

RC HashTable::Delete(int fd, PageNum pageNum)
{
    int bucket = Hash(fd, pageNum);
    HashEntry *entry;
    for (entry = hashTable[bucket]; entry != NULL; entry = entry->next)
    {
        if (entry->fd == fd && entry->pageNum == pageNum)
            break;
    }
    if (entry == NULL)
        return -1;
    if (entry == hashTable[bucket])
        hashTable[bucket] = entry->next;
    if (entry->prev != NULL)
        entry->prev->next = entry->next;
    if (entry->next != NULL)
        entry->next->prev = entry->prev;
    delete entry;
    return 0;
}
