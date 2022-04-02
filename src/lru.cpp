#include "filesystem/lru.h"

namespace kvs
{
void LRU::free(int index)
{
    list->insertFirst(0, index);
}

void LRU::access(int index)
{
    list->insert(0, index);
}

int LRU::find()
{
    int index = list->getFirst(0);
    list->del(index);
    list->insert(0, index);
    return index;
}

LRU::LRU(int c) : CAP_(c)
{
    list = new LinkList(c, 1);
    for (int i = 0; i < CAP_; ++i)
    {
        list->insert(0, i);
    }
}
}  // namespace kvs