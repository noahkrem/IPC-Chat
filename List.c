#include "List.h"
#include <stdio.h>
#include <pthread.h>


static Node nodePool[LIST_MAX_NUM_NODES];
static List listPool[LIST_MAX_NUM_HEADS];
static unsigned int totalNodeCount = 0;    // Keeps track of the total number of nodes used
static unsigned int totalListCount = 0;    // Keeps track of the total number of lists used
static unsigned int nodesFreed = 0;
static unsigned int listsFreed = 0;
static unsigned int unusedNodes[LIST_MAX_NUM_NODES];
static unsigned int unusedLists[LIST_MAX_NUM_HEADS];


// START OF PRIVATE FUNCTIONS -------

// Used when creating a new node
static Node * List_create_node() {

    // This calculation allows us to find free nodes without having to search for them
    unsigned int unusedNodesIndex = (totalNodeCount + nodesFreed) % LIST_MAX_NUM_NODES;
    unsigned int nodePoolIndex = unusedNodes[unusedNodesIndex];
    Node * newNode = &nodePool[nodePoolIndex];
    totalNodeCount++;
    return newNode;
}

// Used when creating a new list
static List * List_create_helper() {

    // This calculation allows us to find free nodes without having to search for them
    unsigned int unusedListsIndex = (totalListCount + listsFreed) % LIST_MAX_NUM_HEADS;
    unsigned int listPoolIndex = unusedLists[unusedListsIndex];
    List * newList = &listPool[listPoolIndex];
    newList->current = NULL;
    newList->head = NULL;
    newList->tail = NULL;
    newList->itemCount = 0;
    newList->oob = LIST_OOB_START;
    totalListCount++;
    return newList;
}

static void List_free_node(Node * node) {

    // Essentially, once we have used the maximum number of nodes, we loop around to the
    //  beginning of the node pool to reuse the nodes that were freed
    unsigned int unusedNodesIndex = node->index;
    // Get rid of all the stored data (except for the index) so that the node can be reused
    node->item = NULL;
    node->next = NULL;
    node->prev = NULL;
    node = NULL;
    unusedNodes[nodesFreed % LIST_MAX_NUM_NODES] = unusedNodesIndex;
    nodesFreed++;
    totalNodeCount--;
}

// This is only called if the list holds zero items
static void List_free_helper(List * list) {

    // Essentially, once we have used the maximum number of lists, we loop around to the
    //  beginning of the list pool to reuse the lists that were freed
    unsigned int unusedListsIndex = list->index;
    list->current = NULL;
    list->head = NULL;
    list->tail = NULL;
    list->itemCount = 0;
    list = NULL;
    unusedLists[listsFreed % LIST_MAX_NUM_HEADS] = unusedListsIndex;
    listsFreed++;
    totalListCount--;
}

// Used whenever inserting into an empty list
static void List_insert_into_empty(List * pList, void * item) {

    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = item;
    pList->head = pList->current;
    pList->tail = pList->current;
    pList->itemCount++;
}

// Used when printing list, for testing purposes
static void List_print(List * pList) {

    Node * temp = pList->head;
    printf("Current contents: ");
    while (temp != NULL) {
        printf("%p ", temp->item);
        temp = temp->next;
    }
    temp = NULL;
    printf("\n");
    printf("Total nodes used: %d\n", totalNodeCount);
}

// END OF PRIVATE FUNCTIONS ---------


// Makes a new, empty list, and returns its reference on success. 
// Returns a NULL pointer on failure.
List* List_create() {

    // If this is the FIRST list to be created, initialize the "unused" arrays
    // Note that these index values should never change after this point
    if ((totalListCount + listsFreed) == 0) {
        for (int i = 0; i < LIST_MAX_NUM_NODES; i++) {
            unusedNodes[i] = i;
            nodePool[i].index = i;
        }
        for (int i = 0; i < LIST_MAX_NUM_HEADS; i++) {
            unusedLists[i] = i;
            listPool[i].index = i;
        }
    }

    if (totalListCount < LIST_MAX_NUM_HEADS) {
        List * newList = List_create_helper();
        return newList;
    }

    // If we already have the max number of lists, return NULL
    return NULL;
}

// Returns the number of items in pList.
int List_count(List* pList) {
    return pList->itemCount;
}

// Returns a pointer to the first item in pList and makes the first item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_first(List* pList) {
    
    if (pList->itemCount == 0) {
        pList->current = NULL;
        return NULL;
    }

    pList->current = pList->head;
    return pList->current->item;
}

// Returns a pointer to the last item in pList and makes the last item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_last(List* pList) {

    if (pList->itemCount == 0) {
        pList->current = NULL;
        return NULL;
    }

    pList->current = pList->tail;
    return pList->current->item;
}

// Advances pList's current item by one, and returns a pointer to the new current item.
// If this operation advances the current item beyond the end of the pList, a NULL pointer 
// is returned and the current item is set to be beyond end of pList.
void* List_next(List* pList) {

    // If we have an empty list...
    if (pList->itemCount == 0) {
        pList->oob = LIST_OOB_END;
        return NULL;
    }

    // If the current item is going beyond the end...
    if (pList->current == pList->tail || (pList->current == NULL && pList->oob == LIST_OOB_END) ) {
        pList->oob = LIST_OOB_END;
        pList->current = NULL;
        return NULL;
    }

    // If the current item is before the start of the list...
    if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = pList->head;
        return pList->current->item;
    }

    pList->current = pList->current->next;
    return pList->current->item;
}

// Backs up pList's current item by one, and returns a pointer to the new current item. 
// If this operation backs up the current item beyond the start of the pList, a NULL pointer 
// is returned and the current item is set to be before the start of pList.
void* List_prev(List* pList) {

    // If we have an empty list...
    if (pList->itemCount == 0) {
        pList->oob = LIST_OOB_START;
        return NULL;
    }

    // If the current item is going before the beginning
    if (pList->current == pList->head || (pList->current == NULL && pList->oob == LIST_OOB_START) ) {
        pList->oob = LIST_OOB_START;
        pList->current = NULL;
        return NULL;
    }

    // If the current item is beyond the end of the list...
    if (pList->current == NULL && pList->oob == LIST_OOB_END) {
        pList->current = pList->tail;
        return pList->current->item;
    }

    pList->current = pList->current->prev;
    return pList->current->item;
}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList) {

    // If the current item is out of bounds, or if the list has no items, return NULL
    if (pList->current == NULL || pList->itemCount == 0)
        return NULL;

    return pList->current->item;
}

// Adds the new item to pList directly after the current item, and makes item the current item. 
// If the current pointer is before the start of the pList, the item is added at the start. If 
// the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert_after(List* pList, void* pItem) {

    // If the available nodes are exhausted
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;

    // If the list is currently empty
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }

    Node * newNode = List_create_node();

    // If the current node is at or beyond the end of the list...
    if (pList->current == pList->tail || (pList->current == NULL && pList->oob == LIST_OOB_END) ) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = NULL;
        pList->current->prev = pList->tail;
        pList->tail->next = pList->current;
        pList->tail = pList->current;
    }
    // If the current node is before the start of the list...
    else if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = pList->head;
        pList->current->prev = NULL;
        pList->head->prev = pList->current;
        pList->head = pList->current;
    }
    // If the current node is none of the above...
    else {
        Node * temp = pList->current;
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = temp->next;
        pList->current->prev = temp;
        temp->next->prev = pList->current;
        temp->next = pList->current;
        temp = NULL;
    }

    pList->itemCount++;
    return LIST_SUCCESS;
}

// Adds item to pList directly before the current item, and makes the new item the current one. 
// If the current pointer is before the start of the pList, the item is added at the start. 
// If the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert_before(List* pList, void* pItem) {
    
    // If the available nodes are exhausted
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;

    // If the list is currently empty
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }

    Node * newNode = List_create_node();

    // If the current node is at or before the start of the list...
    if (pList->current == pList->head || (pList->current == NULL && pList->oob == LIST_OOB_START) ) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = pList->head;
        pList->current->prev = NULL;
        pList->head->prev = pList->current;
        pList->head = pList->current;
    }
    // If the current node is after the end of the list...
    else if (pList->current == NULL && pList->oob == LIST_OOB_END) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = NULL;
        pList->current->prev = pList->tail;
        pList->tail->next = pList->current;
        pList->tail = pList->current;
    }
    // If the current node is none of the above...
    else {
        Node * temp = pList->current;
        Node * tempPrev = pList->current->prev;
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = temp;
        pList->current->prev = temp->prev;
        temp->prev->next = pList->current;
        temp->prev = pList->current;
        temp = NULL;
    }

    pList->itemCount++;
    return LIST_SUCCESS;
}

// Adds item to the end of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_append(List* pList, void* pItem) {
    
    // If the node limit has been reached...
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;


    // If the list is empty...
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }

    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = pItem;
    pList->current->next = NULL;
    pList->current->prev = pList->tail;
    pList->tail->next = pList->current;
    pList->tail = pList->current;

    pList->itemCount++;
    return LIST_SUCCESS;
}

// Adds item to the front of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_prepend(List* pList, void* pItem) {

    // If the node limit has been reached...
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;

    // If the list is empty...
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }

    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = pItem;
    pList->current->next = pList->head;
    pList->current->prev = NULL;
    pList->head->prev = pList->current;
    pList->head = pList->current;

    pList->itemCount++;
    return LIST_SUCCESS;
}

// Return current item and take it out of pList. Make the next item the current one.
// If the current pointer is before the start of the pList, or beyond the end of the pList,
// then do not change the pList and return NULL.
void* List_remove(List* pList) {

    // If the list is empty, or we have any of the conditions mentioned above...
    if (pList->itemCount == 0 || pList->current == NULL) {
        return NULL;
    }

    void * tempItem = pList->current->item;

    // If the list has one node
    if (pList->itemCount == 1) {
        pList->head = NULL;
        pList->tail = NULL;
        List_free_node(pList->current);
    }
    // If the current node is the head
    else if (pList->current == pList->head) {
        pList->head = pList->head->next;
        pList->head->prev = NULL;
        List_free_node(pList->current);
        pList->current = pList->head;
    }
    // If the current node is the tail
    else if (pList->current == pList->tail) {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
        List_free_node(pList->current);
        pList->oob = LIST_OOB_END;
        pList->current = NULL;
    }
    else {
        Node * tempNext = pList->current->next;
        Node * tempPrev = pList->current->prev;
        tempPrev->next = tempNext;
        List_free_node(pList->current);
        pList->current = tempNext;
        pList->current->prev = tempPrev;
    }

    pList->itemCount--;
    return tempItem;
}

// Return last item and take it out of pList. Make the new last item the current one.
// Return NULL if pList is initially empty.
void* List_trim(List* pList) {

    // If the list is empty...
    if (pList->itemCount == 0)
        return NULL;

    void * tempItem = pList->tail->item;
    pList->current = pList->tail;

    // If the list has one node...
    if (pList->itemCount == 1) {
        pList->head = NULL;
        pList->tail = NULL;
        List_free_node(pList->current);
    }
    else {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
        List_free_node(pList->current);
        pList->current = pList->tail;
    }

    pList->itemCount--;
    return tempItem;
}

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1. 
// pList2 no longer exists after the operation; its head is available
// for future operations.
void List_concat(List* pList1, List* pList2) {

    if (pList1->itemCount == 0) {
        pList1->current = pList2->current;
        pList1->head = pList2->head;
        pList1->tail = pList2->tail;
        pList1->itemCount = pList2->itemCount;
        pList1->oob = pList2->oob;
        pList1 = pList2;
        List_free_helper(pList2);
        return;
    }

    if (pList2->itemCount == 0) {
        List_free_helper(pList2);
        return;
    }

    pList1->tail->next = pList2->head;
    pList2->head->prev = pList1->tail;
    pList1->tail = pList2->tail;
    pList1->itemCount += pList2->itemCount;

    List_free_helper(pList2);
}

// Delete pList. pItemFreeFn is a pointer to a routine that frees an item. 
// It should be invoked (within List_free) as: (*pItemFreeFn)(itemToBeFreedFromNode);
// pList and all its nodes no longer exists after the operation; its head and nodes are 
// available for future operations.
typedef void (*FREE_FN)(void* pItem);
void List_free(List* pList, FREE_FN pItemFreeFn) {

    // If the list is not empty...
    if (pList->itemCount != 0) {
        // Free all nodes
        pList->current = pList->head;
        while (pList->current != NULL) {
            (*pItemFreeFn)(pList->current->item);
            List_remove(pList);
            pList->current = pList->head;
        }
    }
    List_free_helper(pList);
}

// Search pList, starting at the current item, until the end is reached or a match is found. 
// In this context, a match is determined by the comparator parameter. This parameter is a
// pointer to a routine that takes as its first argument an item pointer, and as its second 
// argument pComparisonArg. Comparator returns 0 if the item and comparisonArg don't match, 
// or 1 if they do. Exactly what constitutes a match is up to the implementor of comparator. 
// 
// If a match is found, the current pointer is left at the matched item and the pointer to 
// that item is returned. If no match is found, the current pointer is left beyond the end of 
// the list and a NULL pointer is returned.
// 
// If the current pointer is before the start of the pList, then start searching from
// the first node in the list (if any).
typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg) {

    if (pList->current == NULL && pList->oob == LIST_OOB_END)
        return NULL;

    if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = pList->head;
    }

    while (pList->current != NULL) {
        // Check for a match
        if (pComparator(pList->current->item, pComparisonArg) == true)
            return pList->current->item;
        // If no match, advance
        pList->current = pList->current->next;
    }

    // If we reach the end of the list with no match...
    pList->oob = LIST_OOB_END;
    return NULL;
}