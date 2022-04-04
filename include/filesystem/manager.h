#pragma once
#include "../defines.h"

class PageHandle
{
    friend class FileHandle;

public:
    PageHandle();
    ~PageHandle();
    PageHandle(const PageHandle &pageHandle);
    PageHandle &operator=(const PageHandle &pageHandle);

    RC GetData(char *&pData) const;

    RC GetPageNum(PageNum &pageNum) const;

private:
    int pageNum;
    char *pPageData;
};

struct FileHdr
{
    int firstFree;
    int numPages;
};

class BufferManager;

class FileHandle
{
    friend class Manager;

public:
    FileHandle();
    ~FileHandle();
    FileHandle(const FileHandle &fileHandle);

    FileHandle &operator=(const FileHandle &fileHandle);

    RC GetThisPage(PageNum pageNum, PageHandle &pageHandle) const;

    RC AllocatePage(PageHandle &pageHandle);
    RC DisposePage(PageNum pageNum);
    RC MarkDirty(PageNum pageNum) const;
    RC UnpinPage(PageNum pageNum) const;
    RC FlushPages() const;

    RC ForcePages(PageNum pageNum = ALL_PAGES) const;

private:
    int IsValidPageNum(PageNum pageNum) const;

    BufferManager *pBufferManager;
    FileHdr hdr;
    int bFileOpen;
    int bHdrChanged;
    int unixfd;
};

class Manager
{
public:
    Manager();
    ~Manager();
    RC CreateFile(const char *fileName);
    RC DestroyFile(const char *fileName);
    RC OpenFile(const char *fileName, FileHandle &fileHandle);
    RC CloseFile(FileHandle &fileHandle);

private:
    BufferManager *pBufferManager;
};
