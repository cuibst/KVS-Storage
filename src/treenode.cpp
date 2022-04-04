#include "indexsystem/treenode.h"

#include <assert.h>

#include <iostream>

namespace kvs
{

void LeafData::init()
{
    keyOffset = valueOffset = -1;
}

void LeafNode::init()
{
    size = 0;
    leftPage = rightPage = -1;
}

void InternalNode::init()
{
    keyCount = 0;
    for (int i = 0; i <= 2 * D; i++)
        child[i] = -1;
}

void LeafNode::split(LeafNode *splitNode, PageNum newPage, PageNum curPage)
{
    assert(size == 2 * D + 1);
    size = D + 1;
    splitNode->size = D;
    for (int i = 0; i < splitNode->size; i++)
        splitNode->data[i] = data[size + i];
    splitNode->leftPage = curPage;
    splitNode->rightPage = rightPage;
    rightPage = newPage;
}

void LeafNode::insertDataIntoPos(int keyOffset, int valueOffset, int pos)
{
    for (int i = size; i >= pos + 1; i--)
        data[i] = data[i - 1];
    data[pos].keyOffset = keyOffset;
    data[pos].valueOffset = valueOffset;
    // std::cout << "insert into pos = " << pos << std::endl;
    size++;
}

void LeafNode::deleteDataAtPos(int pos)
{
    for (int i = pos; i <= size - 2; i++)
        data[i] = data[i + 1];
    size--;
}

void InternalNode::insertKeyAfterPos(int offset, PageNum pageNum, int pos)
{
    for (int i = keyCount + 1; i >= pos + 2; i--)
    {
        child[i] = child[i - 1];
        offsets[i] = offsets[i - 1];
    }
    child[pos + 1] = pageNum;
    offsets[pos + 1] = offset;
    keyCount++;
}

void InternalNode::deleteKeyAtPos(int pos)
{
    for (int i = pos; i < keyCount; i++)
    {
        child[i] = child[i + 1];
        offsets[i] = offsets[i + 1];
    }
    keyCount--;
}

void InternalNode::split(InternalNode *splitNode)
{
    assert(keyCount == 2 * D);
    keyCount = D;
    splitNode->keyCount = D - 1;
    splitNode->child[0] = child[D + 1];
    splitNode->offsets[0] = offsets[D + 1];
    for (int i = 1; i <= splitNode->keyCount; i++)
    {
        splitNode->child[i] = child[D + 1 + i];
        splitNode->offsets[i] = offsets[D + 1 + i];
    }
}

void LeafNode::operator=(const LeafNode &x)
{
    size = x.size;
    for (int i = 0; i < size; i++)
        data[i] = x.data[i];
    leftPage = x.leftPage;
    rightPage = x.rightPage;
}

}  // namespace kvs