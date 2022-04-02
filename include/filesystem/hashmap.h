#pragma once
#include "defines.h"
#include "linklist.h"

namespace kvs
{
struct Node
{
    int key1, key2;
};

class HashMap
{
private:
    static const int A;
    static const int B;
    const int MOD_, CAP_;
    LinkList *list;
    Node *a;

    int hash(int k1, int k2);

public:
    /**
     * @brief findIndex
     *
     * @param k1 key1
     * @param k2 key2
     * @return int the value in hashMap according to keys, -1 when not found.
     */
    int findIndex(int k1, int k2);

    /**
     * @brief 在hash表中，将指定value对应的两个key设置为k1和k2
     *
     * @param index 指定的value
     * @param k1 指定的第一个key
     * @param k2 指定的第二个key
     */
    void replace(int index, int k1, int k2);

    /**
     * @brief 在hash表中，将指定的value删掉
     *
     * @param index 指定的value
     */
    void remove(int index);

    /**
     * @brief Get the Keys object
     *
     * @param index 指定的value
     * @param k1 存储指定value对应的第一个key
     * @param k2 存储指定value对应的第二个key
     */
    void getKeys(int index, int &k1, int &k2);

    /**
     * @brief Construct a new Hash Map object
     *
     * @param c hash表的容量上限
     * @param m hash函数的mod
     */
    HashMap(int c, int m);

    ~HashMap();
};
}  // namespace kvs