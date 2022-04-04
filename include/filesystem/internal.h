#pragma once

#include <cstdlib>
#include <cstring>

#include "manager.h"

const int BUFFER_SIZE = 40;
const int HASH_TBL_SIZE = 20;

#define CREATION_MASK 0600
#define PAGE_LIST_END -1
#define PAGE_USED -2

struct PageHdr
{
    int nextFree;
};

const int FILE_HDR_SIZE = PAGE_SIZE + sizeof(PageHdr);
