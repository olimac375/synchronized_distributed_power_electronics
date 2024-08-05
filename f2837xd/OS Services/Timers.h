/**
 ********************************************************************************
 * @file    TIMERS_H
 * @brief   
 ********************************************************************************
 */

#ifndef TIMERS_H
#define TIMERS_H

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <stdint.h>
#include "EventsEngine.h"


/************************************
 * MACROS AND DEFINES
 ************************************/
/**
 * @brief   TIMER_GRANULARITY_US is the CPU timer-based time at which the
 *          interrupt is generated.
 * 
 *          It must denote microseconds, as soft-timer remaining time is implicitly declared in microseconds.
 */
#define TIMER_GRANULARITY_US    100UL // 100 microseconds
#define US_PER_SECONDS			1000000UL
/** 
 * Macro for scaling time in seconds to the units used by the soft-timers (microseconds).
 */
#define SECONDS_TO_TICKS(seconds) ((uint32_t)((seconds) * US_PER_SECONDS))

/************************************
 * TYPEDEFS
 ************************************/
/**
 * @brief Defines an soft-timer structure with an associated event.
 * 
 * This structure represents a general propose timer object with an associated event.
 * The callback function to initialized using TimerInit()
 */
typedef struct {
    Event_t Event;      /*!< Event object that contains QueueNode and callback function to notify the application in case of expiration event */
    uint32_t Reload;    /*!< when a timer object is restarted, is loaded with this value */
    uint32_t Remaining; /*!< Time remaining for execution */
} Timer_t;

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

/**
 * @brief   Initialize the timers module.
 *          Initializes CPU Timer 0 to generate an interrupt every TIMER_GRANULARITY_US and
 *          initializes the soft-timer queue (Tqueue).
 */
void Timers_Init(void);

/**
 * @brief   Initialize a soft-timer, defaulted in non-running state.
 * 
 * @param[in] Obj: Pointer to the soft-timer.
 * @param[in] cb: Pointer to the function that will be called upon expiration.
 */
void TimerInit(Timer_t *Obj, Callback_t cb, EventContext_t Context);

/**
 * @brief   Start a soft-timer with the specified period.
 *          if the provided period is less than TIMER_GRANULARITY_US,
 *          the timer will not start, as the timer cannot guarantee that
 *          its execution within the required time.
 * 
 * @param[in] Obj: Pointer to the soft-timer. 
 * @param[in] Period: Time before the soft-timer expires.
 */
void TimerStart(Timer_t *Obj, uint32_t Period);

/**
 * @brief   Stop a soft-timer.
 *          If the soft-timer has expired and is queued for execution, it will be kept
 *          for execution.
 *          @TODO: It would be interesting to be able to remove it from the execution queue.
 * 
 * @param[in] Obj: Pointer to the soft-timer. 
 */
void TimerStop(Timer_t *Obj);

/**
 * @brief   Restart a soft-timer
 *          when a timer has expired and executed, this function can be used to restart the timer
 *          with period previously used.
 * 
 * @param[in] Obj: Pointer to the soft-timer. 
 */
void TimerRestart(Timer_t * Obj);

// @TODO: implement TimerExpired() to check the status of soft-timer

#ifdef __cplusplus
}
#endif

#endif /* MODULE_H */

/*** end of file ***/
