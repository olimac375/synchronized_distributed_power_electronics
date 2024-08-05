/**
 ********************************************************************************
 * @file    EventsEngine.c
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "EventsEngine.h"
#include "cpu.h"


/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/
/**
 * @brief Private typedef to allow internal modification of an event's callback function.
 * 
 * This structure is used internally to manipulate an event's callback function while
 * preventing external modification by users. It has the same structure as Event_t
 * but allows modification to the callback function.
 */
typedef struct
{
    QueueNode_t     QueueNode;      /*!< Queue node information */
    Callback_t      CallbackFunc;   /*!< Actual callback function for event handling, must not be manipulated by users */
    EventContext_t  Context;        /*!< Value determined by the programmer accessible to the callback function*/
} _event_t;
/************************************
 * STATIC VARIABLES
 ************************************/
static Queue_t IsrEventsQueue;  /*!< High priority events queue */
static Queue_t EventsQueue;     /*!< Queue containing all the events scheduled for processing. */

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/
static void Default_EV_Handler(void * args) {
    // Default action, possibly logging or error handling
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void EventInit(Event_t * const Ev, Callback_t const Callback, EventContext_t const Context)
{
    _event_t * const _Ev = (_event_t *)Ev;
    QueueNodeInit((void*)&Ev->QueueNode);
    _Ev->CallbackFunc = (Callback != NULL) ? Callback : Default_EV_Handler;
    _Ev->Context = Context;
}

__attribute__((ramfunc))
void EventPost(Event_t * const Ev)
{
    // enqueue event object 
    QueuePushToTail(&EventsQueue, Ev);  
}

__attribute__((ramfunc))
void EventPostIsr(Event_t * const Ev)
{
    DINT;
    QueuePushToTail(&IsrEventsQueue, Ev);  // enqueue event object 
    EINT;
}

/**
 * @brief Disables global interrupts before popping a queue node object,
 * typically from the IsrEventsQueue. This is done to prevent potential
 * corruption of the queue if the IsrEventsQueue is being manipulated
 * (i.e., new node is being pushed) and another interrupt occurs
 * before the manipulation is completed. 
 * 
 * The function is attributed with __attribute__((ramfunc)) to place and execute 
 * it from RAM, ensuring faster execution.
 * 
 * @param[in] Queue Pointer to the Queue_t structure, typically IsrEventsQueue.
 * 
 * @return Pointer to the dequeued node, or NULL if the queue is empty.
 */
__attribute__((ramfunc))
static inline void * EventPopIsr(Queue_t * const Queue) {
    DINT; // Disable interrupts
    void * Obj = QueuePopFromHead(Queue);
    EINT; // Enable interrupts
    return Obj;
}

__attribute__((ramfunc))
void EventsEngine(void)
{
    Event_t * Ev;
    for(;;) {
        // Process all high priority events
        while ((Ev = EventPopIsr(&IsrEventsQueue)) != NULL) {
                (*Ev->CallbackFunc)(Ev); // Process the task
        }

        // Process the rest of the events
        while ((Ev = QueuePopFromHead(&EventsQueue)) != NULL) {
                (*Ev->CallbackFunc)(Ev); // Process the task
        }
    }
}

void EventsEngineInit(void)
{
    QueueInit(&EventsQueue);
    QueueInit(&IsrEventsQueue);
}

/*** end of file ***/
