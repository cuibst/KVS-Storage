#include "filesystem/internal.h"
#include "filesystem/manager.h"
#define INVALID_PAGE (-1)

PageHandle::PageHandle()
{
    pageNum = INVALID_PAGE;
    pPageData = NULL;
}

PageHandle::~PageHandle()
{
}

PageHandle::PageHandle(const PageHandle &pageHandle)
{
    this->pageNum = pageHandle.pageNum;
    this->pPageData = pageHandle.pPageData;
}

PageHandle &PageHandle::operator=(const PageHandle &pageHandle)
{
    if (this != &pageHandle)
    {
        this->pageNum = pageHandle.pageNum;
        this->pPageData = pageHandle.pPageData;
    }
    return (*this);
}

RC PageHandle::GetData(char *&pData) const
{
    if (pPageData == NULL)
        return -1;
    pData = pPageData;
    return 0;
}

RC PageHandle::GetPageNum(PageNum &_pageNum) const
{
    if (pPageData == NULL)
        return -1;
    _pageNum = this->pageNum;
    return 0;
}
