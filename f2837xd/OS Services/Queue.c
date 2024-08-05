/**
 ********************************************************************************
 * @file    Queue.c
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include <stdbool.h>

#include "Queue.h"
#include "inc\hw_types.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void QueueInit(Queue_t * const Queue)
{
    //	show Queue to be empty
    Queue->FirstNode = (QueueNode_t *)Queue;
    Queue->LastNode = (QueueNode_t *)Queue;
}

void QueueNodeInit(void * const Node)
{
    QueueNode_t * QNode = (QueueNode_t*)Node;

    //	show Node to be unqueued
    QNode->Next  = QNode;
    QNode->Prev  = QNode;
    QNode->Queue = NULL;

}

__attribute__((ramfunc))
void QueuePushToTail(Queue_t * const Queue, void * const Node)
{
    QueueNode_t * PrevNode = (QueueNode_t*)(Queue->LastNode);
    QueueNode_t * QNode = (QueueNode_t*)Node; // Cast the void pointer to a QueueNode_t pointer

    if (Queue == NULL || Node == NULL) return;
    if (QNode->Queue) return;   // Ensure the Node is not already in a queue
    // @TODO: Add a check to avoid adding locally variables that are limited to the scope of its function.

    //New node has been created previously
    //Set Next and Prev pointer of new node and the Previous node
    QNode->Next       = PrevNode->Next;
    QNode->Prev       = PrevNode;
    PrevNode->Next    = QNode;  // Queue is now pointing to the last node added
    QNode->Next->Prev = QNode;

    QNode->Queue      = Queue;  // Node is part of a queue
}

__attribute__((ramfunc))
void * QueueRemove(void * const Node)
{
    QueueNode_t * QNode = (QueueNode_t*)Node;

    if (!QNode) return NULL; 

    QNode->Prev->Next = QNode->Next;
    QNode->Next->Prev = QNode->Prev;

    // Show Node to be unqueued
    QNode->Next       = QNode;
    QNode->Prev       = QNode;
    QNode->Queue = NULL; // Node is not part of a queue

    return QNode;
}

__attribute__((ramfunc))
void * QueuePopFromHead(Queue_t * const Queue)
{
    if (Queue == NULL) return NULL;
    if (IsQueueEmpty(Queue)) return NULL;
    return QueueRemove(Queue->FirstNode);
}

/*** end of file ***/
