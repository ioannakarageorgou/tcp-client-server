//
// Created by jo on 13/5/2019.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "utils.h"

//insert a node at start of the list
void lInsert(ListNode **L, void *A) {
    ListNode *N;

    N = malloc(sizeof(ListNode));
    N->data = A;
    N->next = *L;
    *L = N;
}

//insert at end
void lInsertAtEnd(ListNode**L, void* data) {
    ListNode *N, *P;

    N = malloc(sizeof(ListNode));

    N->data = data;
    N->next = NULL;

    if (*L == NULL) {

        *L = N;

    } else {
        P = *L;
        while (P->next != NULL) {

            P = P->next;

        }

        P->next = N;
    }
}

void DeleteFirstN(ListNode **L) {

    ListNode *temp = *L;
    temp = temp->next;
    free(*L);
    *L = temp;
}


ListNode* DeleteLastN(ListNode* head)
{
    ListNode* temp =head;
    ListNode* t;
    if(head->next==NULL)
    {
        free(head);
        head=NULL;
    }
    else
    {
        while(temp->next != NULL)
        {
            t=temp;
            temp=temp->next;
        }
        free(t->next);
        t->next=NULL;
    }
    return head;
}


void freeList(ListNode *head) {
    ListNode *tmp;

    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void FrontBackSplit(ListNode *source,ListNode**frontRef, ListNode **backRef) {
    ListNode *fast;
    ListNode *slow;
    slow = source;
    fast = source->next;

    /* Advance 'fast' two nodes, and advance 'slow' one node */
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }

    /* 'slow' is before the midpoint in the list, so split it in two
    at that point. */
    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
}

ListNode* SortedMerge(ListNode* a, ListNode* b) {
    ListNode* result = NULL;

    if (a == NULL)
        return (b);
    else if (b == NULL)
        return (a);
    //a->data <= b->data
    if (strcmp((char*)a->data,(char*)b->data) < 0) {
        result = a;
        result->next = SortedMerge(a->next, b);
    } else {
        result = b;
        result->next = SortedMerge(a, b->next);
    }
    return (result);
}

/* sorts the linked list by changing next pointers (not data) */
void MergeSort(ListNode* *headRef) {
    ListNode* head = *headRef;
    ListNode* a;
    ListNode* b;

/* Base case -- length 0 or 1 */
    if ((head == NULL) || (head->next == NULL)) {
        return;
    }

/* Split head into 'a' and 'b' sublists */
    FrontBackSplit(head, &a, &b);

/* Recursively sort the sublists */
    MergeSort(&a);
    MergeSort(&b);

/* answer = merge the two sorted lists together */
    *headRef = SortedMerge(a, b);
}

/* Function to print nodes in a given linked list */
void printList(ListNode* node) {
    printf("printing list: ");
    while (node != NULL) {
        printf("%s ,", (char*)node->data);
        node = node->next;
    }

    printf("\n");
}

void freeListSpecial(ListNode *head) {
    ListNode *tmp;

    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp->data);
        free(tmp);
    }
}

int searchL(ListNode* l, Data *A)
{
    ListNode* P = l;
    while(P != NULL){
        Data* d = P->data;
        if((strcmp(d->IPaddress,A->IPaddress) == 0)&& A->portNum == d->portNum){
            return 0;   //found
        }
        P = P->next;
    }
    return 1;       //not found
}

int sizeL(ListNode* l){

    ListNode* P = l;
    int count = 0;
    while(P != NULL){
        count++;
        P = P->next;
    }
    return count;
}

//void deleteNode(struct Node **head_ref, int key)
//{
//    // Store head node
//    struct Node* temp = *head_ref, *prev;
//
//    // If head node itself holds the key to be deleted
//    if (temp != NULL && temp->data == key)
//    {
//        *head_ref = temp->next;   // Changed head
//        free(temp);               // free old head
//        return;
//    }
//
//    // Search for the key to be deleted, keep track of the
//    // previous node as we need to change 'prev->next'
//    while (temp != NULL && temp->data != key)
//    {
//        prev = temp;
//        temp = temp->next;
//    }
//
//    // If key was not present in linked list
//    if (temp == NULL) return;
//
//    // Unlink the node from linked list
//    prev->next = temp->next;
//
//    free(temp);  // Free memory
//}

void deleteNode(ListNode** l, Data* A)
{
    ListNode* temp = *l,*prev;
    Data * d = temp->data;
    if(temp != NULL && (strcmp(d->IPaddress,A->IPaddress) == 0) && d->portNum == A->portNum){
        // If head node itself holds the key to be deleted
        *l = temp->next;
        free(temp);
        return ;
    }

    while(temp != NULL && d->IPaddress != A->IPaddress && d->portNum != A->portNum){
        prev = temp;
        temp = temp->next;
    }
    // If key was not present in linked list
    if(temp == NULL) return;

    prev->next = temp->next;
    free(temp);
}