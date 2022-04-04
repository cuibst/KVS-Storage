#pragma once
#include <string.h>

#include "../filesystem/defines.h"

namespace kvs
{

struct LeafData
{
    int keyOffset;
    int valueOffset;
    void init();
};

const int D = 50000 / (sizeof(LeafData)) / 4 - 3;

struct LeafNode
{
    int size;
    LeafData data[D * 2 + 1];
    PageNum leftPage, rightPage;
    void init();
    void insertDataIntoPos(int keyOffset, int valueOffset, int pos);
    void deleteDataAtPos(int pos);
    void split(LeafNode *splitNode, PageNum newPage, PageNum curPage);
    void operator=(const LeafNode &);
};

struct InternalNode
{
    int keyCount;
    PageNum child[2 * D + 2];
    int offsets[2 * D + 2];
    void init();
    void insertKeyAfterPos(int offset, PageNum pageNum, int pos);
    void deleteKeyAtPos(int pos);
    void split(InternalNode *splitNode);
};

struct NodePagePacket
{
    NodeType type;
    LeafNode leafNode;
    InternalNode internalNode;
};

}  // namespace kvs