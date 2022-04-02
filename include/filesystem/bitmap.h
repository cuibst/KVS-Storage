#pragma once

#include "defines.h"

#define LEAF_BIT 32
#define MAX_LEVEL 5
#define MAX_INNER_NUM 67
#define BIAS 5

namespace kvs
{
class BitMap
{
protected:
    static unsigned getMask(int k);

    uint *data;
    int size;
    int rootBit;
    int rootLevel;
    int rootIndex;
    uint inner[MAX_INNER_NUM];
    uint innerMask;
    uint rootMask;

    uint getLeafData(int index);
    void setLeafData(int index, unsigned v);
    int setLeafBit(int index, unsigned k);
    unsigned childWord(int start, int bitNum, int i, int j);
    void init();
    int _setBit(unsigned *start, int index, unsigned k);
    void updateInner(
        int level, int offset, int index, int levelCap, unsigned k);
    int _findLeftOne(int level, int offset, int pos, int prevLevelCap);

public:
    static int _hash(unsigned i);
    static void initConst();
    static int getIndex(unsigned k);
    static unsigned lowbit(unsigned k);
    static void getPos(int index, int &pos, int &bit);
    unsigned data0();
    void setBit(int index, unsigned k);
    int findLeftOne();
    BitMap(int cap, unsigned k);
    BitMap(int cap, unsigned *da);
    void reload(unsigned *da);
};
}  // namespace kvs