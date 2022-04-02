#pragma once

namespace kvs
{
class LinkList
{
private:
    struct ListNode
    {
        int next;
        int prev;
    };
    int cap;
    int listNum;
    ListNode *a;
    void link(int prev, int next);

public:
    void del(int index);
    void insert(int listID, int ele);
    void insertFirst(int listID, int ele);
    int getFirst(int listID);
    int next(int index);
    bool isHead(int index);
    bool isAlone(int index);
    LinkList(int c, int n);
};
}  // namespace kvs