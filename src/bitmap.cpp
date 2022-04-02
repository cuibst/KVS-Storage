#include "filesystem/bitmap.h"

namespace kvs
{
static unsigned char h[61];

unsigned BitMap::getMask(int k)
{
    unsigned s = 0;
    for (int i = 0; i < k; i++)
        s += (1 << i);
    return s;
}

uint BitMap::getLeafData(int index)
{
    return data[index];
}

void BitMap::setLeafData(int index, unsigned v)
{
    data[index] = v;
}

int BitMap::setLeafBit(int index, unsigned k)
{
    int pos, bit;
    getPos(index, pos, bit);
    unsigned umask = (1 << bit);
    unsigned mask = ~umask;
    if (k == 0)
        umask = 0;
    unsigned w = ((getLeafData(pos) & mask) | umask);
    setLeafData(pos, w);
    return pos;
}

uint BitMap::childWord(int start, int bitNum, int i, int j)
{
    // cout << start << " " << bitNum << " " << i << " " << j << endl;
    int index = (i << BIAS) + j;
    if (start == 0)
    {
        return getLeafData(index);
    }
    else
    {
        // cout << start - bitNum + index << endl;
        return inner[start - bitNum + index];
    }
}

void BitMap::init()
{
    rootLevel = 0;
    int s = size;
    rootIndex = 0;
    while (s > LEAF_BIT)
    {
        int wordNum = (s >> BIAS);
        // cout << rootIndex << " " << s << " " << wordNum << endl;
        for (int i = 0; i < wordNum; ++i)
        {
            uint w = 0;
            // cout << "---------------------------------------" << endl;
            for (int j = 0; j < LEAF_BIT; ++j)
            {
                // cout << i << endl;
                uint k = (1 << j);
                uint c = childWord(rootIndex, s, i, j);
                if (c != 0)
                {
                    w += k;
                }
            }
            inner[rootIndex + i] = w;
        }
        rootLevel++;
        rootIndex += wordNum;
        s = wordNum;
    }
    rootBit = s;
    int i = 0;
    uint w = 0;
    for (int j = 0; j < rootBit; ++j)
    {
        uint k = (1 << j);
        uint c = childWord(rootIndex, s, i, j);
        if (c != 0)
        {
            w += k;
        }
    }
    inner[rootIndex] = w;
    innerMask = getMask(BIAS);
    rootMask = getMask(s);
}

int BitMap::_setBit(uint *start, int index, uint k)
{
    int pos, bit;
    getPos(index, pos, bit);
    uint umask = (1 << bit);
    uint mask = (~umask);
    if (k == 0)
    {
        umask = 0;
    }
    start[pos] = ((start[pos] & mask) | umask);
    return pos;
}

void BitMap::updateInner(int level, int offset, int index, int levelCap, uint k)
{
    // cout << level << " " << rootLevel << endl;
    uint *start = (&inner[offset]);
    int pos = _setBit(start, index, k);
    if (level == rootLevel)
    {
        return;
    }
    uint c = 1;
    if (start[pos] == 0)
    {
        c = 0;
    }
    /*
    if (level == 1) {
            cout << "level1:" << start[index] << " " << c << endl;
    }*/
    updateInner(level + 1, offset + levelCap, pos, (levelCap >> BIAS), c);
}

int BitMap::_findLeftOne(int level, int offset, int pos, int prevLevelCap)
{
    uint lb = lowbit(inner[offset + pos]);
    int index = h[_hash(lb)];
    /*if (level == 0) {
            cout << "level0:" << index << " " << pos << endl;
    }*/
    int nPos = (pos << BIAS) + index;
    if (level == 0)
    {
        //	cout << "npos " << nPos << endl;
        return nPos;
    }
    return _findLeftOne(
        level - 1, offset - prevLevelCap, nPos, (prevLevelCap << BIAS));
}

int BitMap::_hash(uint i)
{
    return i % 61;
}

void BitMap::initConst()
{
    for (int i = 0; i < 32; ++i)
    {
        unsigned int k = (1 << i);
        h[_hash(k)] = i;
    }
}

int BitMap::getIndex(uint k)
{
    return h[_hash(k)];
}

uint BitMap::lowbit(uint k)
{
    return (k & (-k));
}

void BitMap::getPos(int index, int &pos, int &bit)
{
    pos = (index >> BIAS);
    bit = index - (pos << BIAS);
}

uint BitMap::data0()
{
    return data[0];
}

void BitMap::setBit(int index, uint k)
{
    // cout << data[0]<<endl;
    int p = setLeafBit(index, k);
    // cout <<"seting "<<data[0]<<endl;
    // cout << "ok" << endl;
    uint c = 1;
    if (getLeafData(p) == 0)
    {
        c = 0;
    }
    // cout << p << " " << c << endl;
    updateInner(0, 0, p, (size >> BIAS), c);
}

int BitMap::findLeftOne()
{
    int i = _findLeftOne(rootLevel, rootIndex, 0, rootBit);
    /*
    for (i = 0; i < size;++i){
            if (data[i] !=0)break;
    }*/
    // cout << "nPosi " << i << " " << getLeafData(i) << endl;
    // cout << i << endl;
    // cout << data[0] << endl;
    uint lb = lowbit(getLeafData(i));
    int index = h[_hash(lb)];
    return (i << BIAS) + index;
}

BitMap::BitMap(int cap, uint k)
{
    initConst();
    size = (cap >> BIAS);
    data = new uint[size];
    uint fill = 0;
    if (k == 1)
    {
        fill = 0xffffffff;
    }
    for (int i = 0; i < size; ++i)
    {
        data[i] = fill;
    }
    init();
}

BitMap::BitMap(int cap, uint *da)
{
    initConst();
    data = da;
    size = (cap >> BIAS);
    init();
}

void BitMap::reload(uint *da)
{
    data = da;
}
}  // namespace kvs
