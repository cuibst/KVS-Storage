#pragma once

#include "bitmap.h"
#include "defines.h"
#include "filemanager.h"
#include "hashmap.h"
#include "lru.h"

namespace kvs
{
class BufPageManager
{
private:
    int last;
    FileManager *fileManager;
    HashMap *hash;
    LRU *replace;
    bool *dirty;
    BufType *addr;
    BufType allocMem();
    BufType fetchPage(int typeID, int pageID, int &index);

public:
    /**
     * @brief 为文件中的某一个页面获取一个缓存中的页面
     *        缓存中的页面在缓存页面数组中的下标记录在index中
     *        并根据ifRead是否为true决定是否将文件中的内容写到获取的缓存页面中
     *
     *        注意:在调用函数allocPage之前，调用者必须确信(fileID,pageID)指定的文件页面不存在缓存中
     *        如果确信指定的文件页面不在缓存中，那么就不用在hash表中进行查找，直接调用替换算法，节省时间
     *
     * @param fileID
     * 文件id，数据库程序在运行时，用文件id来区分正在打开的不同的文件
     * @param pageID 文件页号，表示在fileID指定的文件中，第几个文件页
     * @param index 函数返回时，用来记录缓存页面数组中的下标
     * @param ifRead 是否要将文件页中的内容读到缓存中
     * @return BufType 缓存页面的首地址
     */
    BufType allocPage(int fileID, int pageID, int &index, bool ifRead = false);

    /**
     * @brief 为文件中的某一个页面在缓存中找到对应的缓存页面
     *           文件页面由(fileID,pageID)指定
     *           缓存中的页面在缓存页面数组中的下标记录在index中
     *           首先，在hash表中查找(fileID,pageID)对应的缓存页面，
     *           如果能找到，那么表示文件页面在缓存中
     *           如果没有找到，那么就利用替换算法获取一个页面
     *
     * @param fileID 文件id
     * @param pageID 文件页号
     * @param index 函数返回时，用来记录缓存页面数组中的下标
     * @return BufType 缓存页面的首地址
     */
    BufType getPage(int fileID, int pageID, int &index);

    /**
     * @brief 标记index代表的缓存页面被访问过，为替换算法提供信息
     *
     * @param index 缓存页面数组中的下标，用来表示一个缓存页面
     */
    void access(int index);

    /**
     * @brief
     * 标记index代表的缓存页面被写过，保证替换算法在执行时能进行必要的写回操作，
     *           保证数据的正确性
     *
     * @param index 缓存页面数组中的下标，用来表示一个缓存页面
     */
    void markDirty(int index);

    /**
     * @brief
     * 将index代表的缓存页面归还给缓存管理器，在归还前，缓存页面中的数据不标记写回
     *
     * @param index 缓存页面数组中的下标，用来表示一个缓存页面
     */
    void release(int index);

    /**
     * @brief
     * 将index代表的缓存页面归还给缓存管理器，在归还前，缓存页面中的数据需要根据脏页标记决定是否写到对应的文件页面中
     *
     * @param index 缓存页面数组中的下标，用来表示一个缓存页面
     */
    void writeBack(int index);

    /**
     * @brief
     * 将所有缓存页面归还给缓存管理器，归还前需要根据脏页标记决定是否写到对应的文件页面中
     *
     */
    void close();

    /**
     * @brief Get the Key object
     *
     * @param index 缓存页面数组中的下标，用来指定一个缓存页面
     * @param fileID 函数返回时，用于存储指定缓存页面所属的文件号
     * @param pageID 函数返回时，用于存储指定缓存页面对应的文件页号
     */
    void getKey(int index, int &fileID, int &pageID);

    /**
     * @brief Construct a new Buf Page Manager object
     *
     * @param fm 文件管理器，缓存管理器需要利用文件管理器与磁盘进行交互
     */
    BufPageManager(FileManager *fm);

    ~BufPageManager();
};
}  // namespace kvs