#pragma once

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef int RC;
typedef long PageNum;

const int ALL_PAGES = -1;
const int PAGE_SIZE = (65536 - sizeof(int));

enum NodeType
{
    ROLeaf,
    ROInternal,
    Leaf,
    Internal
};
