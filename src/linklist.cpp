#include "filesystem/linklist.h"
namespace kvs
{
void LinkList::link(int prev, int next)
{
    a[prev].next = next;
    a[next].prev = prev;
}

void LinkList::del(int index)
{
    if (a[index].prev == index)
        return;
    link(a[index].prev, a[index].next);
    a[index].prev = index;
    a[index].next = index;
}

void LinkList::insert(int listID, int ele)
{
    del(ele);
    int node = listID + cap;
    int prev = a[node].prev;
    link(prev, ele);
    link(ele, node);
}

void LinkList::insertFirst(int listID, int ele)
{
    del(ele);
    int node = listID + cap;
    int next = a[node].next;
    link(node, ele);
    link(ele, next);
}

int LinkList::getFirst(int listID)
{
    return a[listID + cap].next;
}

int LinkList::next(int index)
{
    return a[index].next;
}

bool LinkList::isHead(int index)
{
    return index >= cap;
}

bool LinkList::isAlone(int index)
{
    return (a[index].next == index);
}

LinkList::LinkList(int c, int n)
{
    cap = c;
    listNum = n;
    a = new ListNode[n + c];
    for (int i = 0; i < cap + listNum; ++i)
    {
        a[i].next = i;
        a[i].prev = i;
    }
}
}  // namespace kvs