#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agui.h"

/* ─────────────────────────────────────────────
   Data payload stored in each node
   ───────────────────────────────────────────── */
typedef struct {
	int type;
    int r;
    int c;
    int w;
    int h;
    int color;
	int line_dir;
    BorderType style;
    char text[256];
} NodeData;

/* ─────────────────────────────────────────────
   Linked-list node
   ───────────────────────────────────────────── */
typedef struct ListNode {
    NodeData        data;
    struct ListNode *next;
} ListNode;

/* ─────────────────────────────────────────────
   List handle  (keeps head + size together)
   ───────────────────────────────────────────── */
typedef struct {
    ListNode *head;
    size_t    size;
} LinkedList;

/* ── Lifecycle ─────────────────────────────── */
void    listInit    (LinkedList *list);
void    listDestroy (LinkedList *list);

/* ── Insert ────────────────────────────────── */
int     listPushFront (LinkedList *list, NodeData data);             /* O(1) */
int     listPushBack  (LinkedList *list, NodeData data);             /* O(n) */
int     listInsertAt  (LinkedList *list, size_t index, NodeData data); /* O(n) */

/* ── Remove ────────────────────────────────── */
int     listPopFront   (LinkedList *list);                           /* O(1) */
int     listPopBack    (LinkedList *list);                           /* O(n) */
int     listRemoveAt   (LinkedList *list, size_t index);             /* O(n) */
int     listRemoveById (LinkedList *list, int id);                   /* O(n) */

/* ── Search ────────────────────────────────── */
ListNode *listFindById (const LinkedList *list, int id);
ListNode *listGetAt    (const LinkedList *list, size_t index);

/* ── Utility ───────────────────────────────── */
void    listPrint   (const LinkedList *list);
void    listReverse (LinkedList *list);
size_t  listSize    (const LinkedList *list);
int     listIsEmpty (const LinkedList *list);

#endif /* LINKED_LIST_H */
