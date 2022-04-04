#include "engine.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <mutex>

#include "indexsystem/indexmanager.h"

namespace kvs
{

int Engine::logFileDescriptor = -1;
unsigned Engine::offset = 0;

static std::mutex mt;

std::string Engine::loadKey(unsigned keyOffset)
{
    assert(logFileDescriptor != -1);
    off_t error = lseek(logFileDescriptor, keyOffset, SEEK_SET);
    assert(error == keyOffset);
    unsigned char *tmp = new unsigned char[kMaxKeySize + 1];
    error = read(logFileDescriptor, (void *) tmp, kMaxKeySize + 1);
    int p = 0;
    std::string key = "";
    while (!(tmp[p] & 128))
        key.push_back((char) tmp[p++]);
    delete[] tmp;
    return key;
}

std::string Engine::loadValue(unsigned valueOffset)
{
    assert(logFileDescriptor != -1);
    off_t error = lseek(logFileDescriptor, valueOffset, SEEK_SET);
    assert(error == valueOffset);
    unsigned char *tmp = new unsigned char[kMaxValueSize + 1];
    error = read(logFileDescriptor, (void *) tmp, kMaxValueSize + 1);
    int p = 0;
    std::string value = "";
    while (tmp[p] != 255)
        value.push_back((char) tmp[p++]);
    delete[] tmp;
    return value;
}

void Engine::loadKeyValue(Key &key, Value &value, unsigned keyOffset)
{
    assert(logFileDescriptor != -1);
    off_t error = lseek(logFileDescriptor, keyOffset, SEEK_SET);
    assert(error == keyOffset);
    unsigned char *tmp = new unsigned char[kMaxKeySize + kMaxValueSize + 2];
    error =
        read(logFileDescriptor, (void *) tmp, kMaxKeySize + kMaxValueSize + 2);
    int p = 0;
    while (!(tmp[p] & 128))
        key.push_back((char) tmp[p++]);
    p++;
    while (!(tmp[p] & 128))
        value.push_back((char) tmp[p++]);
}

void Engine::setRootPageNum(PageNum num)
{
    rootPageNum = num;
}

Engine::Engine(const std::string &path, EngineOptions options)
    : indexManager(IndexManager::getInstance(path + "/" + "kvs.index")),
      path(path),
      options(options)
{
    // TODO: your code here
    std::ignore = path;
    std::ignore = options;

    rootPageNum = -1;

    std::string name = path + "/" + "kvslog.log";
    if (logFileDescriptor == -1)
    {
        mt.lock();
        if (logFileDescriptor == -1)
        {
            FILE *f = fopen(name.c_str(), "a+");
            assert(f != NULL);
            fclose(f);
            logFileDescriptor = open(name.c_str(), O_RDWR);
            f = fopen(name.c_str(), "r");
            std::string keyBuf;
            bool flag = false;
            unsigned char x;
            unsigned storeOffset = 0;
            while (fscanf(f, "%c", &x) != EOF)
            {
                offset++;
                if (x == 254)
                {
                    indexManager.insertEntry(keyBuf, storeOffset, offset);
                    unsigned tmp;
                    keyBuf = "";
                    flag = true;
                }
                else if (x == 255)
                {
                    flag = false;
                    storeOffset = offset;
                }
                else if (x == 253)
                {
                    indexManager.deleteEntry(keyBuf, storeOffset);
                    keyBuf = "";
                    storeOffset = offset;
                }
                else if (!flag)
                    keyBuf.push_back(x);
            }
            logFileDescriptor = open(name.c_str(), O_RDWR);
            assert(logFileDescriptor != -1);
        }
        mt.unlock();
    }
}

Engine::~Engine()
{
    // TODO: your code here
}

RetCode Engine::put(const Key &key, const Value &value)
{
    // TODO: your code here
    std::ignore = key;
    std::ignore = value;
    unsigned char *tmp = new unsigned char[key.length() + value.length() + 2];
    for (int i = 0; i < key.length(); i++)
        tmp[i] = key[i];
    tmp[key.length()] = 254;
    for (int i = 0; i < value.length(); i++)
        tmp[key.length() + i + 1] = value[i];
    tmp[key.length() + value.length() + 1] = 255;
    mt.lock();
    // std::cout << "put " << key << " " << offset << std::endl;
    off_t error = lseek(logFileDescriptor, offset, SEEK_SET);
    assert(error == offset);
    unsigned storeOffset = offset;
    error = write(
        logFileDescriptor, (void *) tmp, key.length() + value.length() + 2);
    offset += key.length() + 1;
    indexManager.insertEntry(key, storeOffset, offset);
    offset += value.length() + 1;
    mt.unlock();
    delete[] tmp;
    return RetCode::kSucc;
    // return kNotSupported;
}
RetCode Engine::remove(const Key &key)
{
    // TODO: your code here
    std::ignore = key;
    mt.lock();
    // std::cout << "remove " << key << " " << offset << std::endl;
    unsigned offset1;
    bool res = indexManager.getEntry(key, offset1);
    if (res)
    {
        unsigned char *tmp = new unsigned char[key.length() + 1];
        for (int i = 0; i < key.length(); i++)
            tmp[i] = key[i];
        tmp[key.length()] = 253;
        off_t error = lseek(logFileDescriptor, offset, SEEK_SET);
        assert(error == offset);
        error = write(logFileDescriptor, (void *) tmp, key.length() + 1);
        indexManager.deleteEntry(key, offset);
        offset += key.length() + 1;
        delete[] tmp;
    }
    mt.unlock();
    return res ? RetCode::kSucc : RetCode::kNotFound;
    // return kNotSupported;
}

RetCode Engine::get(const Key &key, Value &value)
{
    // TODO: your code here
    std::ignore = key;
    std::ignore = value;
    mt.lock();
    // std::cout << "get " << key << " " << offset << std::endl;
    unsigned valueOffset;
    bool flag = indexManager.getEntry(key, valueOffset, this->rootPageNum);
    if (!flag)
    {
        mt.unlock();
        return RetCode::kNotFound;
    }
    value = loadValue(valueOffset);
    mt.unlock();
    return RetCode::kSucc;
    // return kNotSupported;
}

RetCode Engine::sync()
{
    mt.lock();
    mt.unlock();
    return RetCode::kSucc;
    // return kNotSupported;
}

RetCode Engine::visit(const Key &lower,
                      const Key &upper,
                      const Visitor &visitor)
{
    // TODO: your code here
    // std::ignore = lower;
    // std::ignore = upper;
    // std::ignore = visitor;
    // return kNotSupported;
    mt.lock();
    indexManager.visit(lower, upper, visitor, this->rootPageNum);
    mt.unlock();
    return RetCode::kSucc;
}

RetCode Engine::garbage_collect()
{
    std::string newLogPath = path + "/" + "kvslog_cpy.log";
    FILE *file = fopen(newLogPath.c_str(), "a+");
    fclose(file);
    std::string oldLogPath = path + "/" + "kvslog.log";
    // TODO: your code here
    mt.lock();
    offset = indexManager.rebuildLog(path);
    close(logFileDescriptor);
    unlink(oldLogPath.c_str());
    rename(newLogPath.c_str(), oldLogPath.c_str());
    logFileDescriptor = open(oldLogPath.c_str(), O_RDWR);
    mt.unlock();
    return RetCode::kSucc;
}

std::shared_ptr<IROEngine> Engine::snapshot()
{
    // TODO: your code here
    // std::cout << "snapshot generated" << std::endl;
    auto res = std::make_shared<Engine>(path, options);
    mt.lock();
    res->setRootPageNum(indexManager.persistent());
    mt.unlock();
    return res;
}

}  // namespace kvs
