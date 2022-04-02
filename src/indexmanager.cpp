#include "indexsystem/indexmanager.h"

#include <mutex>

namespace kvs
{

static std::mutex mt;

IndexManager::IndexManager()
{
}

IndexManager &IndexManager::getInstance()
{
    static IndexManager im;
    return im;
}

void IndexManager::put(std::string key, unsigned offset)
{
    mt.lock();
    indices[key] = offset;
    mt.unlock();
}

bool IndexManager::get(std::string key, unsigned &offset)
{
    mt.lock();
    if (indices.count(key))
    {
        offset = indices[key];
        mt.unlock();
        return true;
    }
    mt.unlock();
    return false;
}

bool IndexManager::remove(std::string key)
{
    bool flag = false;
    mt.lock();
    if (indices.count(key))
    {
        indices.erase(key);
        flag = true;
    }
    mt.unlock();
    return flag;
}
}  // namespace kvs