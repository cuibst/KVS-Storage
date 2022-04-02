#include "filesystem/filemanager.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
using namespace std;
namespace kvs
{
int FileManager::_createFile(const char *name)
{
    FILE *f = fopen(name, "a+");
    if (f == NULL)
    {
        cout << "fail" << endl;
        return -1;
    }
    fclose(f);
    return 0;
}

int FileManager::_openFile(const char *name, int fileID)
{
    int f = open(name, O_RDWR);
    if (f == -1)
    {
        return -1;
    }
    fd[fileID] = f;
    return 0;
}

FileManager::FileManager()
{
    fm = new BitMap(MAX_FILE_NUM, 1);
    tm = new BitMap(MAX_TYPE_NUM, 1);
}

int FileManager::writePage(int fileID, int pageID, BufType buf, int off)
{
    int f = fd[fileID];
    off_t offset = pageID;
    offset = (offset << PAGE_SIZE_IDX);
    off_t error = lseek(f, offset, SEEK_SET);
    if (error != offset)
    {
        return -1;
    }
    BufType b = buf + off;
    error = write(f, (void *) b, PAGE_SIZE);
    return 0;
}

int FileManager::readPage(int fileID, int pageID, BufType buf, int off)
{
    // int f = fd[fID[type]];
    int f = fd[fileID];
    off_t offset = pageID;
    offset = (offset << PAGE_SIZE_IDX);
    off_t error = lseek(f, offset, SEEK_SET);
    if (error != offset)
    {
        return -1;
    }
    BufType b = buf + off;
    error = read(f, (void *) b, PAGE_SIZE);
    return 0;
}

int FileManager::closeFile(int fileID)
{
    fm->setBit(fileID, 1);
    int f = fd[fileID];
    close(f);
    return 0;
}

bool FileManager::createFile(const char *name)
{
    _createFile(name);
    return true;
}

bool FileManager::openFile(const char *name, int &fileID)
{
    fileID = fm->findLeftOne();
    fm->setBit(fileID, 0);
    _openFile(name, fileID);
    return true;
}

int FileManager::newType()
{
    int t = tm->findLeftOne();
    tm->setBit(t, 0);
    return t;
}

void FileManager::closeType(int typeID)
{
    tm->setBit(typeID, 1);
}

void FileManager::shutdown()
{
    delete tm;
    delete fm;
}

FileManager::~FileManager()
{
    this->shutdown();
}
}  // namespace kvs