#pragma once

#include "bitmap.h"
#include "defines.h"

namespace kvs
{
class FileManager
{
private:
    int fd[MAX_FILE_NUM];
    BitMap *fm;
    BitMap *tm;
    int _createFile(const char *name);
    int _openFile(const char *name, int fileID);
    FileManager();

public:
    /**
     * @brief
     * 将buf+off开始的2048个四字节整数(8kb信息)写入fileID和pageID指定的文件页中
     *
     * @param fileID 文件id，用于区别已经打开的文件
     * @param pageID 文件的页号
     * @param buf 存储信息的缓存(4字节无符号整数数组)
     * @param off 偏移量
     * @return int 成功操作返回0
     */
    int writePage(int fileID, int pageID, BufType buf, int off);

    /**
     * @brief
     * 将fileID和pageID指定的文件页中2048个四字节整数(8kb)读入到buf+off开始的内存中
     *
     * @param fileID 文件id，用于区别已经打开的文件
     * @param pageID 文件页号
     * @param buf 存储信息的缓存(4字节无符号整数数组)
     * @param off 偏移量
     * @return int 成功操作返回0
     */
    int readPage(int fileID, int pageID, BufType buf, int off);

    /**
     * @brief 关闭文件
     *
     * @param fileID 用于区别已经打开的文件
     * @return int 操作成功，返回0
     */
    int closeFile(int fileID);

    /**
     * @brief 新建name指定的文件名
     *
     * @param name 文件名
     * @return true 操作成功，返回true
     * @return false
     */
    bool createFile(const char *name);

    /**
     * @brief 打开文件
     *
     * @param name 文件名
     * @param fileID
     * 函数返回时，如果成功打开文件，那么为该文件分配一个id，记录在fileID中
     * @return true
     * 如果成功打开，在fileID中存储为该文件分配的id，返回true，否则返回false
     * @return false
     */
    bool openFile(const char *name, int &fileID);

    int newType();
    void closeType(int typeID);
    void shutdown();
    ~FileManager();
};
}  // namespace kvs