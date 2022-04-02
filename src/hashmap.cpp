#include "filesystem/hashmap.h"

namespace kvs
{

const int HashMap::A = 1;
const int HashMap::B = 1;

int HashMap::hash(int k1, int k2)
{
    return (k1 * A + k2 * B) % MOD_;
}

int HashMap::findIndex(int k1, int k2)
{
    int h = hash(k1, k2);
    int p = list->getFirst(h);
    while (!list->isHead(p))
    {
        if (a[p].key1 == k1 && a[p].key2 == k2)
        {
            /*
            list.del(p);
            list.insertFirst(p);
            */
            return p;
        }
        p = list->next(p);
    }
    return -1;
}

void HashMap::replace(int index, int k1, int k2)
{
    int h = hash(k1, k2);
    // cout << h << endl;
    list->insertFirst(h, index);
    a[index].key1 = k1;
    a[index].key2 = k2;
}

void HashMap::remove(int index)
{
    list->del(index);
    a[index].key1 = -1;
    a[index].key2 = -1;
}

void HashMap::getKeys(int index, int &k1, int &k2)
{
    k1 = a[index].key1;
    k2 = a[index].key2;
}

HashMap::HashMap(int c, int m) : CAP_(c), MOD_(m)
{
    a = new Node[c];
    for (int i = 0; i < CAP_; ++i)
    {
        a[i].key1 = -1;
        a[i].key2 = -1;
    }
    list = new LinkList(CAP_, MOD_);
}

HashMap::~HashMap()
{
    delete[] a;
    delete list;
}
}  // namespace kvs