/**
 ********************************************************************************
 * @file    EventsEngine.h
 * @brief   This library provides services to posting events and for eventually handling the events' job
 ********************************************************************************
 */

#ifndef EVENTSENGINE_H
#define EVENTSENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include <stdbool.h>
#include "Queue.h"
#include "inc\hw_types.h"

/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/

typedef void (*Callback_t)(void * args);    /*!< Event callback function that handles a specific event, taking a pointer to function arguments as parameters. */

typedef uint32_t EventContext_t;            /*!< Convenient custom event context information. Its usage depends on the programmer's discretion. */

/**
 * @brief Defines an event structure with a read-only callback function and associated context.
 * 
 * This structure represents an event with an associated callback function and context,
 * intended for use in event-driven systems. The callback function, once assigned during
 * initialization using EventInit(), remains read-only to users to prevent unauthorized
 * modification.
 */
typedef struct
{
    QueueNode_t         QueueNode;      /*!< Queue node information */
    Callback_t const    CallbackFunc;   /*!< (read-only) The event handler, use EventInit to assign callback function */
    EventContext_t      Context;        /*!< Value determined by the programmer accessible to the callback function */
} Event_t;

/**
 * @note In order to maintain clarity and consistency between declaration for constant variables and constant pointer 
 * the 'const' qualifier will apper after the type specifier ( i.e. 'init' or 'int *') and before the variable or pointer name. For example:
 * 
 * int const Data_const;
 * int * const Pointer_const;
 * int cont * const dataNpointer_const;
 */

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

/**
 * @brief       Initializes an event with a specified callback function and context.
 * 
 * It make sure that the callback function is either set to the provided function or to a default
 * handler if no valid function pointer is provided. CallbackFunc is read-only to users to 
 * maintain the integrity of the event. 
 * 
 * @param[in]   Ev: Pointer to the Event_t structure.
 * @param[in]   Func: Callback function associated with the event.
 * @param[in]   Context: Context information for the event.
 * 
 * @return      None
 */
void EventInit(Event_t * const Ev, Callback_t const Func, EventContext_t const Context);

/**
 * @brief       Posts an event, triggering the associated callback function.
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Ev: Pointer to the Event_t structure.
 * 
 * @return      void
 */
void EventPost(Event_t * const Ev);

/**
 * @brief       Posts a high-priority event (typically an event posted by an interrupt), 
 *              triggering the associated callback function.
 * 
 *              This function must be used when running in an interrupt context, as it ensures
 *              safe manipulation of queues by disabling and enabling global interrupts to push
 *              a new object onto the queue.
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   Ev: Pointer to the Event_t structure.
 * 
 * @return      void
 */
void EventPostIsr(Event_t * const Ev);

/**
 * @brief       Manages the event engine, processing pending events. 
 *              This function should be called within an infinite loop to handle all pending events.
 * 
 *              The function is attributed with __attribute__((ramfunc)) to place and execute 
 *              it from RAM, ensuring faster execution.
 * 
 * @param[in]   None.
 * 
 * @return      void
 */
void EventsEngine(void);

/**
 * @brief       Initializes the events engine.
 * @param[in]   None.
 * 
 * @return      void
 */
void EventsEngineInit(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_H */

/*** end of file ***/
