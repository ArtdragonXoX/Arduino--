#pragma once
#include <stdint.h>
#include <stdlib.h>

struct ListNode
{
    uint8_t x, y;
    ListNode *nextNode;
    ListNode *iter;

    void AddX()
    {
        InitIter();
        while (Iter())
        {
            iter->x++;
            NextIter();
        }
    }

    void SubX()
    {
        InitIter();
        while (Iter())
        {
            iter->x--;
            NextIter();
        }
    }

    void AddY()
    {
        InitIter();
        while (Iter())
        {
            iter->y++;
            NextIter();
        }
    }

    void SubY()
    {
        InitIter();
        while (Iter())
        {
            iter->y--;
            NextIter();
        }
    }

    ListNode *Front()
    {
        if (this->nextNode)
            return this->nextNode;
        return nullptr;
    }

    ListNode *Back()
    {
        ListNode *node = this;
        while (node->nextNode)
            node = node->nextNode;
        return node;
    }

    void RemoveBack()
    {
        if (!this->nextNode)
            return;
        ListNode *node = this;
        while (node->nextNode->nextNode)
            node = node->nextNode;
        free(node->nextNode);
        node->nextNode = nullptr;
    }

    void CopyBack()
    {
        ListNode *back = this->Back();
        ListNode *newNode = (ListNode *)malloc(sizeof(ListNode));
        newNode->x = back->x;
        newNode->y = back->y;
        newNode->nextNode = nullptr;
        back->nextNode = newNode;
    }

    void PushFront(ListNode *node)
    {
        node->nextNode = this->nextNode;
        this->nextNode = node;
    }

    void PushFront(uint8_t x, uint8_t y)
    {
        ListNode *node = (ListNode *)malloc(sizeof(ListNode));
        node->x = x;
        node->y = y;
        node->nextNode = nullptr;
        PushFront(node);
    }

    void PushBack(ListNode *node)
    {
        this->Back()->nextNode = node;
    }

    void PushBack(uint8_t x, uint8_t y)
    {
        ListNode *node = (ListNode *)malloc(sizeof(ListNode));
        node->x = x;
        node->y = y;
        node->nextNode = nullptr;
        PushBack(node);
    }

    void RemoveAll()
    {
        ListNode *node = this->nextNode;
        while (node)
        {
            ListNode *freeNode = node;
            node = node->nextNode;
            free(freeNode);
        }
        nextNode = nullptr;
    }

    void InitIter()
    {
        iter = this->nextNode;
    }

    void NextIter()
    {
        if (iter)
            iter = iter->nextNode;
    }

    ListNode *Iter()
    {
        return iter;
    }

    uint8_t Count()
    {
        ListNode *node = this->nextNode;
        uint8_t res = 0;
        while (node)
        {
            node = node->nextNode;
            res++;
        }
        return res;
    }
};

typedef ListNode List;

List *InitList()
{
    List *list = (List *)malloc(sizeof(List));
    list->x = 0;
    list->y = 0;
    list->nextNode = nullptr;
    return list;
}
