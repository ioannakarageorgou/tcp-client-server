//
// Created by jo on 16/5/2019.
//

#ifndef UNTITLED_LIST_H
#define UNTITLED_LIST_H

#include "structures.h"

typedef struct ListNode{

    void* data;
    struct ListNode* next;

}ListNode;

void lInsert(ListNode**L, void* data);
void DeleteFirstN(ListNode ** );
void freeList(ListNode*);
void FrontBackSplit(ListNode *source,ListNode**frontRef, ListNode **backRef);
ListNode* SortedMerge(ListNode* a, ListNode* b);
void MergeSort(ListNode* *headRef);
void printList(ListNode* node);
void lInsertAtEnd(ListNode**L, void* data);
ListNode* DeleteLastN(ListNode* head);
void freeListSpecial(ListNode *head);
int searchL(ListNode* l, Data *A);
int sizeL(ListNode* l);
void deleteNode(ListNode** l, Data* A);

#endif //UNTITLED_LIST_H
