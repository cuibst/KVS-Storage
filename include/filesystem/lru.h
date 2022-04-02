#pragma once

#include "defines.h"
#include "hashmap.h"
#include "linklist.h"

namespace kvs
{
class LRU
{
private:
    LinkList *list;
    const int CAP_;

public:
    /**
     * @brief 将缓存页面数组中第index个页面的缓存空间回收
     *        下一次通过find函数寻找替换页面时，直接返回index
     *
     * @param index 缓存页面数组中页面的下标
     */
    void free(int index);

    /**
     * @brief 将缓存页面数组中第index个页面标记为访问
     *
     * @param index 缓存页面数组中页面的下标
     */
    void access(int index);

    /**
     * @brief 根据替换算法返回缓存页面数组中要被替换页面的下标
     *
     * @return int
     */
    int find();

    /**
     * @brief Construct a new LRU object
     *
     * @param c 表示缓存页面的容量上限
     */
    LRU(int c);
};
}  // namespace kvs