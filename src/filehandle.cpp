#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "filesystem/buffermanager.h"
#include "filesystem/internal.h"

FileHandle::FileHandle()
{
    bFileOpen = 0;
    pBufferManager = NULL;
}

FileHandle::~FileHandle()
{
}

FileHandle::FileHandle(const FileHandle &fileHandle)
{
    this->pBufferManager = fileHandle.pBufferManager;
    this->hdr = fileHandle.hdr;
    this->bFileOpen = fileHandle.bFileOpen;
    this->bHdrChanged = fileHandle.bHdrChanged;
    this->unixfd = fileHandle.unixfd;
}

FileHandle &FileHandle::operator=(const FileHandle &fileHandle)
{
    if (this != &fileHandle)
    {
        this->pBufferManager = fileHandle.pBufferManager;
        this->hdr = fileHandle.hdr;
        this->bFileOpen = fileHandle.bFileOpen;
        this->bHdrChanged = fileHandle.bHdrChanged;
        this->unixfd = fileHandle.unixfd;
    }
    return (*this);
}

RC FileHandle::GetThisPage(PageNum pageNum, PageHandle &pageHandle) const
{
    int rc;
    char *pPageBuf;

    if (!bFileOpen)
        return -1;

    if (!IsValidPageNum(pageNum))
        return -1;

    if ((rc = pBufferManager->GetPage(unixfd, pageNum, &pPageBuf)))
        return rc;

    if (((PageHdr *) pPageBuf)->nextFree == PAGE_USED)
    {
        pageHandle.pageNum = pageNum;
        pageHandle.pPageData = pPageBuf + sizeof(PageHdr);
        return 0;
    }
    if ((rc = UnpinPage(pageNum)))
        return rc;
    return -1;
}

RC FileHandle::AllocatePage(PageHandle &pageHandle)
{
    int rc;
    int pageNum;
    char *pPageBuf;

    if (!bFileOpen)
        return -1;

    if (hdr.firstFree != PAGE_LIST_END)
    {
        pageNum = hdr.firstFree;
        if ((rc = pBufferManager->GetPage(unixfd, pageNum, &pPageBuf)))
            return rc;
        hdr.firstFree = ((PageHdr *) pPageBuf)->nextFree;
    }
    else
    {
        pageNum = hdr.numPages;
        if ((rc = pBufferManager->AllocatePage(unixfd, pageNum, &pPageBuf)))
            return rc;
        hdr.numPages++;
    }
    bHdrChanged = 1;
    ((PageHdr *) pPageBuf)->nextFree = PAGE_USED;
    memset(pPageBuf + sizeof(PageHdr), 0, PAGE_SIZE);
    if ((rc = MarkDirty(pageNum)))
        return rc;
    pageHandle.pageNum = pageNum;
    pageHandle.pPageData = pPageBuf + sizeof(PageHdr);
    return 0;
}

RC FileHandle::DisposePage(PageNum pageNum)
{
    int rc;
    char *pPageBuf;
    if (!bFileOpen)
        return -1;
    if (!IsValidPageNum(pageNum))
        return -1;
    if ((rc = pBufferManager->GetPage(unixfd, pageNum, &pPageBuf, 0)))
        return rc;
    if (((PageHdr *) pPageBuf)->nextFree != PAGE_USED)
    {
        if ((rc = UnpinPage(pageNum)))
            return rc;
        return -1;
    }
    ((PageHdr *) pPageBuf)->nextFree = hdr.firstFree;
    hdr.firstFree = pageNum;
    bHdrChanged = 1;
    if ((rc = MarkDirty(pageNum)))
        return rc;

    if ((rc = UnpinPage(pageNum)))
        return rc;
    return 0;
}

RC FileHandle::MarkDirty(PageNum pageNum) const
{
    if (!bFileOpen)
        return -1;
    if (!IsValidPageNum(pageNum))
        return -1;
    return (pBufferManager->MarkDirty(unixfd, pageNum));
}

RC FileHandle::UnpinPage(PageNum pageNum) const
{
    if (!bFileOpen)
        return -1;
    if (!IsValidPageNum(pageNum))
        return -1;
    return (pBufferManager->UnpinPage(unixfd, pageNum));
}

RC FileHandle::FlushPages() const
{
    if (!bFileOpen)
        return -1;
    if (bHdrChanged)
    {
        if (lseek(unixfd, 0, L_SET) < 0)
            return -1;
        int numBytes = write(unixfd, (char *) &hdr, sizeof(FileHdr));
        if (numBytes < 0)
            return -1;
        if (numBytes != sizeof(FileHdr))
            return -1;
        FileHandle *dummy = (FileHandle *) this;
        dummy->bHdrChanged = 0;
    }
    return (pBufferManager->FlushPages(unixfd));
}

RC FileHandle::ForcePages(PageNum pageNum) const
{
    if (!bFileOpen)
        return -1;
    if (bHdrChanged)
    {
        if (lseek(unixfd, 0, L_SET) < 0)
            return -1;
        int numBytes = write(unixfd, (char *) &hdr, sizeof(FileHdr));
        if (numBytes < 0)
            return -1;
        if (numBytes != sizeof(FileHdr))
            return -1;
        FileHandle *dummy = (FileHandle *) this;
        dummy->bHdrChanged = 0;
    }
    return (pBufferManager->ForcePages(unixfd, pageNum));
}

int FileHandle::IsValidPageNum(PageNum pageNum) const
{
    return (bFileOpen && pageNum >= 0 && pageNum < hdr.numPages);
}
