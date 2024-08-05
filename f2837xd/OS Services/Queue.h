/**
 ********************************************************************************
 * @file    Queue.h
 * @brief   Queue library provide service to queue up objects, FIFO type only support for now.
 * 
 *          Quick reference: https://en.wikipedia.org/wiki/Doubly_linked_list
 ********************************************************************************
 */

#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include <stdbool.h>
#include "inc\hw_types.h"


/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/
typedef struct QueueNode_t
{
    struct QueueNode_t * Next;      /*!< A reference to the next node */
    struct QueueNode_t * Prev;      /*!< A reference to the previous node */
    struct Queue_t     * Queue;     /*!< A reference to the queue*/
}QueueNode_t;

typedef struct Queue_t
{
    struct QueueNode_t * FirstNode; /*!< Points to the first node of the queue; points to itself when queue is empty*/
    struct QueueNode_t * LastNode;  /*!< Points to the last node of the queue; points to itself when queue is empty*/
}Queue_t;

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
/**
 * @brief       Initializes a doubly linked queue object.
 * @param[in]   Queue: Pointer to the Queue_t structure.
 * 
 * @return      void
 */
void QueueInit(Queue_t * const Queue);

/**
 * @brief       Initializes a queue node before being linked into a queue object.
 * @param[in]   Node: Pointer to the Queue node.
 * 
 * @return      void
 */
void QueueNodeInit(void * const Node);

// @TODO: Increase queue capability by implementing the following methods:
// void QueuePushToHead(Queue_t *Queue, void *Node);
// void QueuePushBefore(Queue_t *Queue, void *Node);
// void QueuePushAfter(Queue_t *Queue, void *Node);
// void *QueuePopFromTail(Queue_t *Queue);

/**
 * @brief       Puts a node into the queue.
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Queue: Pointer to the Queue_t structure.
 * @param[in]   Node: Pointer to the node to be added to the queue.
 * 
 * @return      void
 */
void QueuePushToTail(Queue_t * const Queue, void * const Node);

/**
 * @brief       Retrieves and removes a node from the head of the queue.
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Queue: Pointer to the Queue_t structure.
 * 
 * @return      Pointer to the dequeued node, or NULL if the queue is empty.
 */
void * QueuePopFromHead(Queue_t * const Queue);

/**
 * @brief       Retrieves and removes given node
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Queue: Pointer to the Queue_t structure.
 * 
 * @return      Pointer to the dequeued node, or NULL if the queue is empty.
 */
void * QueueRemove(void * const Node);

/**
 * @brief       Checks if the queue is empty.
 * @param[in]   Queue: Pointer to the Queue_t structure.
 * 
 * @return      true if the queue is empty, false otherwise.
 * 
 * @note        This is a static inline function defined in a header file because it can then be
 *              inlined at function calls. The compiler needs to know the body of a function to 
 *              be able to inline it. It is declared static to avoid multiple definitions at linking time.
 *              In general, it is a good practice to include short functions as static inline definitions
 *              in common header files.
 */
static inline bool IsQueueEmpty(Queue_t * const Queue)
{
    return (Queue->FirstNode == (QueueNode_t*)Queue);
}

/**
 * @brief       Returns the first node in the queue (the head).
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Queue: Pointer to the Queue structure.
 * @return      If the queue is not empty, returns a pointer to the head of the queue; otherwise, returns NULL.
 */
static inline void * GetQueueHead(Queue_t * const Queue) {
    return IsQueueEmpty(Queue) ? NULL : Queue->FirstNode;
}

/**
 * @brief       Returns the node that follows the current node.
 * 
 * @param[in]   Node: Pointer to a node.
 * @return      If the node is not the last node in the queue (the tail) and is actually part of a queue,
 *              returns a pointer to the following node; otherwise, returns NULL.
 */
static inline void * GetNextNode(void * const Node) {
    QueueNode_t * Queue = (QueueNode_t *)(((QueueNode_t *)Node)->Queue); // which queue this node belongs to
    QueueNode_t * NextNode = ((QueueNode_t *)Node)->Next;

    if(Queue == NULL || NextNode == Queue) { // If the node is unqueued or already at the last node of the queue.
        return NULL; 
    }

    return NextNode;    // return next node
}

/**
 * @brief       Check if a node is part of a queue.
 * 
 * @param[in]   Queue: Pointer to the queue.
 * @param[in]   Node: Pointer to the node being evaluated.
 * @return      True if the node is part of the given queue; otherwise, False.
 */
static inline bool IsNodeInQueue(Queue_t *Queue, void *Node) {
    return ((QueueNode_t *)Node)->Queue == Queue;
}

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */

/*** end of file ***/
