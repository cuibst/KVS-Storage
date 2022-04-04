#include "filesystem/manager.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>

#include "filesystem/buffermanager.h"
#include "filesystem/internal.h"

Manager::Manager()
{
    pBufferManager = new BufferManager(BUFFER_SIZE);
}

Manager::~Manager()
{
    delete pBufferManager;
}

RC Manager::CreateFile(const char *fileName)
{
    int fd;
    int numBytes;

    if ((fd = open(fileName, O_CREAT | O_EXCL | O_WRONLY, CREATION_MASK)) < 0)
        return -1;

    char hdrBuf[FILE_HDR_SIZE];

    memset(hdrBuf, 0, FILE_HDR_SIZE);

    FileHdr *hdr = (FileHdr *) hdrBuf;
    hdr->firstFree = PAGE_LIST_END;
    hdr->numPages = 0;

    if ((numBytes = write(fd, hdrBuf, FILE_HDR_SIZE)) != FILE_HDR_SIZE)
    {
        close(fd);
        unlink(fileName);

        if (numBytes < 0)
            return -1;
        else
            return -1;
    }

    if (close(fd) < 0)
        return -1;

    return 0;
}

RC Manager::DestroyFile(const char *fileName)
{
    if (unlink(fileName) < 0)
        return -1;

    return 0;
}

RC Manager::OpenFile(const char *fileName, FileHandle &fileHandle)
{
    int rc;

    if (fileHandle.bFileOpen)
        return -1;

    if ((fileHandle.unixfd = open(fileName, O_RDWR)) < 0)
        return -1;

    {
        int numBytes =
            read(fileHandle.unixfd, (char *) &fileHandle.hdr, sizeof(FileHdr));
        if (numBytes != sizeof(FileHdr))
        {
            rc = (numBytes < 0) ? -1 : -1;
            goto err;
        }
    }

    fileHandle.bHdrChanged = 0;

    fileHandle.pBufferManager = pBufferManager;
    fileHandle.bFileOpen = 1;

    return 0;

err:
    close(fileHandle.unixfd);
    fileHandle.bFileOpen = 0;

    return rc;
}

RC Manager::CloseFile(FileHandle &fileHandle)
{
    RC rc;
    if (!fileHandle.bFileOpen)
        return -1;
    if ((rc = fileHandle.FlushPages()))
        return rc;
    if (close(fileHandle.unixfd) < 0)
        return -1;
    fileHandle.bFileOpen = 0;
    fileHandle.pBufferManager = NULL;
    return 0;
}
