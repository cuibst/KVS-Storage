#include "engine.h"

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <mutex>

namespace kvs
{

int Engine::logFileDescriptor = -1;
IndexManager &Engine::indexManager = IndexManager::getInstance();
unsigned Engine::offset = 0;

static std::mutex mt;

Engine::Engine(const std::string &path, EngineOptions options)
{
    // TODO: your code here
    std::ignore = path;
    std::ignore = options;

    std::string name = path + "/" + "kvslog.log";
    if (logFileDescriptor == -1)
    {
        mt.lock();
        if (logFileDescriptor == -1)
        {
            FILE *f = fopen(name.c_str(), "a+");
            assert(f != NULL);
            fclose(f);
            f = fopen(name.c_str(), "r");
            std::string keyBuf;
            bool flag = false;
            unsigned char x;
            while (fscanf(f, "%c", &x) != EOF)
            {
                offset++;
                if (x == 254)
                {
                    indexManager.put(keyBuf, offset);
                    keyBuf = "";
                    flag = true;
                }
                else if (x == 255)
                    flag = false;
                else if (x == 253)
                {
                    indexManager.remove(keyBuf);
                    keyBuf = "";
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
    off_t error = lseek(logFileDescriptor, offset, SEEK_SET);
    assert(error == offset);
    error = write(
        logFileDescriptor, (void *) tmp, key.length() + value.length() + 2);
    offset += key.length() + 1;
    indexManager.put(key, offset);
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
    bool res = indexManager.remove(key);
    if (res)
    {
        unsigned char *tmp = new unsigned char[key.length() + 1];
        for (int i = 0; i < key.length(); i++)
            tmp[i] = key[i];
        tmp[key.length()] = 253;
        off_t error = lseek(logFileDescriptor, offset, SEEK_SET);
        assert(error == offset);
        error = write(logFileDescriptor, (void *) tmp, key.length() + 1);
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
    unsigned valueOffset;
    bool flag = indexManager.get(key, valueOffset);
    if (!flag)
    {
        mt.unlock();
        return RetCode::kNotFound;
    }
    off_t error = lseek(logFileDescriptor, valueOffset, SEEK_SET);
    if (error != valueOffset)
        return RetCode::kNotFound;
    unsigned char *tmp = new unsigned char[kMaxValueSize + 1];
    error = read(logFileDescriptor, (void *) tmp, kMaxKeyValueSize + 1);
    int p = 0;
    value = "";
    while (tmp[p] != 255)
        value.push_back((char) tmp[p++]);
    mt.unlock();
    delete[] tmp;
    return RetCode::kSucc;
    // return kNotSupported;
}

RetCode Engine::sync()
{
    return RetCode::kSucc;
    // return kNotSupported;
}

RetCode Engine::visit(const Key &lower,
                      const Key &upper,
                      const Visitor &visitor)
{
    // TODO: your code here
    std::ignore = lower;
    std::ignore = upper;
    std::ignore = visitor;
    return kNotSupported;
}

RetCode Engine::garbage_collect()
{
    // TODO: your code here
    return kNotSupported;
}

std::shared_ptr<IROEngine> Engine::snapshot()
{
    // TODO: your code here
    return nullptr;
}

}  // namespace kvs
