#include "filesystem/buffermanager.h"

#include <unistd.h>

#include <cstdio>
#include <iostream>

using namespace std;

BufferManager::BufferManager(int _numPages) : hashTable(HASH_TBL_SIZE)
{
    this->numPages = _numPages;
    pageSize = PAGE_SIZE + sizeof(PageHdr);

    bufTable = new BufPageDesc[numPages];
    for (int i = 0; i < numPages; i++)
    {
        if ((bufTable[i].pData = new char[pageSize]) == NULL)
            exit(1);

        memset((void *) bufTable[i].pData, 0, pageSize);

        bufTable[i].prev = i - 1;
        bufTable[i].next = i + 1;
    }
    bufTable[0].prev = bufTable[numPages - 1].next = INVALID_SLOT;
    free = 0;
    first = last = INVALID_SLOT;
}

BufferManager::~BufferManager()
{
    for (int i = 0; i < this->numPages; i++)
        delete[] bufTable[i].pData;

    delete[] bufTable;
}

RC BufferManager::GetPage(int fd,
                          PageNum pageNum,
                          char **ppBuffer,
                          int bMultiplePins)
{
    RC rc;
    int slot;

    if ((rc = hashTable.Find(fd, pageNum, slot)) && (rc != -1))
        return rc;  // unexpected error

    if (rc == -1)
    {
        if ((rc = InternalAlloc(slot)))
            return rc;

        if ((rc = ReadPage(fd, pageNum, bufTable[slot].pData)) ||
            (rc = hashTable.Insert(fd, pageNum, slot)) ||
            (rc = InitPageDesc(fd, pageNum, slot)))
        {
            Unlink(slot);
            InsertFree(slot);
            return rc;
        }
    }
    else
    {
        if (!bMultiplePins && bufTable[slot].pinCount > 0)
            return -1;

        bufTable[slot].pinCount++;
        // Make this page the most recently used page
        if ((rc = Unlink(slot)) || (rc = LinkHead(slot)))
            return rc;
    }

    *ppBuffer = bufTable[slot].pData;

    return 0;
}

RC BufferManager::AllocatePage(int fd, PageNum pageNum, char **ppBuffer)
{
    RC rc;
    int slot;

    if (!(rc = hashTable.Find(fd, pageNum, slot)))
        return -1;
    else if (rc != -1)
        return rc;
    if ((rc = InternalAlloc(slot)))
        return rc;
    if ((rc = hashTable.Insert(fd, pageNum, slot)) ||
        (rc = InitPageDesc(fd, pageNum, slot)))
    {
        Unlink(slot);
        InsertFree(slot);
        return rc;
    }

    *ppBuffer = bufTable[slot].pData;
    return 0;
}

RC BufferManager::MarkDirty(int fd, PageNum pageNum)
{
    RC rc;
    int slot;

    if ((rc = hashTable.Find(fd, pageNum, slot)))
        return rc;

    if (bufTable[slot].pinCount == 0)
        return -1;

    bufTable[slot].bDirty = 1;

    if ((rc = Unlink(slot)) || (rc = LinkHead(slot)))
        return rc;

    return 0;
}

RC BufferManager::UnpinPage(int fd, PageNum pageNum)
{
    RC rc;
    int slot;

    if ((rc = hashTable.Find(fd, pageNum, slot)))
        return rc;

    if (bufTable[slot].pinCount == 0)
        return -1;

    if (--(bufTable[slot].pinCount) == 0)
        if ((rc = Unlink(slot)) || (rc = LinkHead(slot)))
            return rc;

    return 0;
}

RC BufferManager::FlushPages(int fd)
{
    RC rc, rcWarn = 0;

    int slot = first;
    while (slot != INVALID_SLOT)
    {
        int next = bufTable[slot].next;
        if (bufTable[slot].fd == fd)
        {
            if (bufTable[slot].pinCount)
            {
                rcWarn = -1;
            }
            else
            {
                if (bufTable[slot].bDirty)
                {
                    if ((rc = WritePage(
                             fd, bufTable[slot].pageNum, bufTable[slot].pData)))
                        return rc;
                    bufTable[slot].bDirty = 0;
                }
                if ((rc = hashTable.Delete(fd, bufTable[slot].pageNum)) ||
                    (rc = Unlink(slot)) || (rc = InsertFree(slot)))
                    return rc;
            }
        }
        slot = next;
    }

    return rcWarn;
}

RC BufferManager::ForcePages(int fd, PageNum pageNum)
{
    RC rc;

    int slot = first;
    while (slot != INVALID_SLOT)
    {
        int next = bufTable[slot].next;

        if (bufTable[slot].fd == fd &&
            (pageNum == ALL_PAGES || bufTable[slot].pageNum == pageNum))
        {
            if (bufTable[slot].bDirty)
            {
                if ((rc = WritePage(
                         fd, bufTable[slot].pageNum, bufTable[slot].pData)))
                    return rc;
                bufTable[slot].bDirty = 0;
            }
        }
        slot = next;
    }

    return 0;
}

RC BufferManager::InsertFree(int slot)
{
    bufTable[slot].next = free;
    free = slot;
    return 0;
}

RC BufferManager::LinkHead(int slot)
{
    bufTable[slot].next = first;
    bufTable[slot].prev = INVALID_SLOT;

    if (first != INVALID_SLOT)
        bufTable[first].prev = slot;

    first = slot;

    if (last == INVALID_SLOT)
        last = first;
    return 0;
}

RC BufferManager::Unlink(int slot)
{
    if (first == slot)
        first = bufTable[slot].next;
    if (last == slot)
        last = bufTable[slot].prev;
    if (bufTable[slot].next != INVALID_SLOT)
        bufTable[bufTable[slot].next].prev = bufTable[slot].prev;
    if (bufTable[slot].prev != INVALID_SLOT)
        bufTable[bufTable[slot].prev].next = bufTable[slot].next;
    bufTable[slot].prev = bufTable[slot].next = INVALID_SLOT;
    return 0;
}

RC BufferManager::InternalAlloc(int &slot)
{
    RC rc;
    if (free != INVALID_SLOT)
    {
        slot = free;
        free = bufTable[slot].next;
    }
    else
    {
        for (slot = last; slot != INVALID_SLOT; slot = bufTable[slot].prev)
        {
            if (bufTable[slot].pinCount == 0)
                break;
        }
        if (slot == INVALID_SLOT)
            return -1;
        if (bufTable[slot].bDirty)
        {
            if ((rc = WritePage(bufTable[slot].fd,
                                bufTable[slot].pageNum,
                                bufTable[slot].pData)))
                return rc;

            bufTable[slot].bDirty = 0;
        }
        if ((rc =
                 hashTable.Delete(bufTable[slot].fd, bufTable[slot].pageNum)) ||
            (rc = Unlink(slot)))
            return rc;
    }
    if ((rc = LinkHead(slot)))
        return rc;
    return 0;
}

RC BufferManager::ReadPage(int fd, PageNum pageNum, char *dest)
{
    long offset = pageNum * (long) pageSize + FILE_HDR_SIZE;
    if (lseek(fd, offset, L_SET) < 0)
        return -1;
    int numBytes = read(fd, dest, pageSize);
    if (numBytes < 0)
        return -1;
    else if (numBytes != pageSize)
    {
        printf("read %d bytes but expected %d bytes\n", numBytes, pageSize);
        return -1;
    }
    else
        return 0;
}

RC BufferManager::WritePage(int fd, PageNum pageNum, char *source)
{
    long offset = pageNum * (long) pageSize + FILE_HDR_SIZE;
    if (lseek(fd, offset, L_SET) < 0)
        return -1;

    int numBytes = write(fd, source, pageSize);
    if (numBytes < 0)
        return -1;
    else if (numBytes != pageSize)
        return -1;
    else
        return 0;
}

RC BufferManager::InitPageDesc(int fd, PageNum pageNum, int slot)
{
    bufTable[slot].fd = fd;
    bufTable[slot].pageNum = pageNum;
    bufTable[slot].bDirty = 0;
    bufTable[slot].pinCount = 1;
    return 0;
}
