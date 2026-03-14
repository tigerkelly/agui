#include "linkedList.h"

/* ════════════════════════════════════════════
   Internal helper – allocate and fill a node
   ════════════════════════════════════════════ */
static ListNode *nodeCreate(NodeData data) {
    ListNode *n = (ListNode *)malloc(sizeof(ListNode));
    if (!n) {
        fprintf(stderr, "[linkedList] malloc failed\n");
        return NULL;
    }
    n->data = data;
    n->next = NULL;
    return n;
}

/* ════════════════════════════════════════════
   Lifecycle
   ════════════════════════════════════════════ */

/* Initialise an empty list. Must be called before any other function. */
void listInit(LinkedList *list) {
    if (!list) return;
    list->head = NULL;
    list->size = 0;
}

/* Free every node and reset the list to empty. */
void listDestroy(LinkedList *list) {
    if (!list) return;
    ListNode *cur = list->head;
    while (cur) {
        ListNode *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    list->head = NULL;
    list->size = 0;
}

/* ════════════════════════════════════════════
   Insert
   ════════════════════════════════════════════ */

/* Insert at the front – O(1).
   Returns 0 on success, -1 on allocation failure. */
int listPushFront(LinkedList *list, NodeData data) {
    if (!list)
		return -1;
    ListNode *n = nodeCreate(data);
    if (!n)
		return -1;

    n->next    = list->head;
    list->head = n;
    list->size++;
    return 0;
}

/* ════════════════════════════════════════════
   Utility
   ════════════════════════════════════════════ */

/* Print every node to stdout. */
void listPrint(const LinkedList *list) {
    if (!list || !list->head) {
        printf("[empty list]\n");
        return;
    }
    printf("List (%zu nodes):\n", list->size);
    ListNode *cur = list->head;
    size_t    i   = 0;
    while (cur) {
        printf("  [%zu] type=%c  x=%d y=%d w=%d h=%d text=%s\n",
               i++, cur->data.type, cur->data.r, cur->data.c, cur->data.w, cur->data.h, cur->data.text);
        cur = cur->next;
    }
}

/* Append to the back – O(n).
   Returns 0 on success, -1 on allocation failure. */
int listPushBack(LinkedList *list, NodeData data) {
    if (!list)
		return -1;
    ListNode *n = nodeCreate(data);
    if (!n)
		return -1;

    if (!list->head) {
        list->head = n;
    } else {
        ListNode *cur = list->head;
        while (cur->next)
			cur = cur->next;
        cur->next = n;
    }
    list->size++;
    return 0;
}

/* Insert before the node currently at `index` (0-based) – O(n).
   Appends when index >= size.
   Returns 0 on success, -1 on failure. */
int listInsertAt(LinkedList *list, size_t index, NodeData data) {
    if (!list)
		return -1;

    /* Delegate edge cases */
    if (index == 0)
		return listPushFront(list, data);
    if (index >= list->size)
		return listPushBack (list, data);

    ListNode *n = nodeCreate(data);
    if (!n)
		return -1;

    ListNode *cur = list->head;
    for (size_t i = 0; i < index - 1; i++)
        cur = cur->next;

    n->next   = cur->next;
    cur->next = n;
    list->size++;
    return 0;
}

/* ════════════════════════════════════════════
   Remove
   ════════════════════════════════════════════ */

/* Remove the first node – O(1).
   Returns 0 on success, -1 if the list is empty. */
int listPopFront(LinkedList *list) {
    if (!list || !list->head)
		return -1;

    ListNode *tmp = list->head;
    list->head    = tmp->next;
    free(tmp);
    list->size--;
    return 0;
}

/* Remove the last node – O(n).
   Returns 0 on success, -1 if the list is empty. */
int listPopBack(LinkedList *list)
{
    if (!list || !list->head)
		return -1;

    if (!list->head->next) {           /* single node */
        free(list->head);
        list->head = NULL;
    } else {
        ListNode *cur = list->head;
        while (cur->next->next)
			cur = cur->next;
        free(cur->next);
        cur->next = NULL;
    }
    list->size--;
    return 0;
}

/* Remove the node at `index` (0-based) – O(n).
   Returns 0 on success, -1 on out-of-range or empty list. */
int listRemoveAt(LinkedList *list, size_t index) {
    if (!list || !list->head || index >= list->size)
		return -1;
    if (index == 0)
		return listPopFront(list);

    ListNode *cur = list->head;
    for (size_t i = 0; i < index - 1; i++) {
        cur = cur->next;
	}

    ListNode *tmp = cur->next;
    cur->next     = tmp->next;
    free(tmp);
    list->size--;
    return 0;
}

/* Return a pointer to the node at `index`, or NULL if out-of-range. */
ListNode *listGetAt(const LinkedList *list, size_t index) {
    if (!list || index >= list->size)
		return NULL;
    ListNode *cur = list->head;
    for (size_t i = 0; i < index; i++) {
		cur = cur->next;
	}
    return cur;
}

/* Reverse the list in-place – O(n). */
void listReverse(LinkedList *list) {
    if (!list || list->size < 2)
		return;
    ListNode *prev = NULL;
    ListNode *cur  = list->head;
    while (cur) {
        ListNode *next = cur->next;
        cur->next      = prev;
        prev           = cur;
        cur            = next;
    }
    list->head = prev;
}

/* Return the number of nodes. */
size_t listSize(const LinkedList *list) {
    return list ? list->size : 0;
}

/* Return 1 if the list is empty, 0 otherwise. */
int listIsEmpty(const LinkedList *list) {
    return (!list || list->size == 0) ? 1 : 0;
}
