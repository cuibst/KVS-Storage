#pragma once

#include <map>

namespace kvs
{
class IndexManager
{
private:
    IndexManager();
    std::map<std::string, unsigned> indices;

public:
    static IndexManager &getInstance();
    void put(std::string key, unsigned offset);
    bool get(std::string key, unsigned &offset);
    bool remove(std::string key);
};
}  // namespace kvs