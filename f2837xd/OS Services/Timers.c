/**
 ********************************************************************************
 * @file    Timers.c
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include <string.h>
#include "device.h"
#include "EventsEngine.h"
#include "Timers.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define SYS_TIME_BASE 			(uint32_t)(US_PER_SECONDS / TIMER_GRANULARITY_US)
#define SYS_TICKS    			(uint32_t)(DEVICE_SYSCLK_FREQ / SYS_TIME_BASE) // calculate hardware ticks based on TIMER_GRANULARITY_US

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static uint32_t timebase; // in microseconds
static Event_t cpuTimer0EventISR;

static Queue_t Tqueue; //queue of soft-timers
/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
__interrupt void cpuTimer0ISR(void);
static void timeBaseHandler(void * args);

/************************************
 * STATIC FUNCTIONS
 ************************************/
// cpuTimer0ISR takes approximately 540ns-560ns to execute.
__attribute__((ramfunc))
__interrupt void cpuTimer0ISR(void) {
    // CPU-timer counter register (TIM) is automatically loaded with the value in the period register (PRD)
    EventPostIsr(&cpuTimer0EventISR);
    // Acknowledge this interrupt to receive more interrupts from group 1
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

/**
 * @brief   Timer interrupt handler. This function iterates through all the soft-timers
 *          queued, decrements their remaining time for execution, and triggers events
 *          when timers expire.
 * 
 *          timeBaseHandler takes approximately 2.8us to execute, @TODO: PFM-1435 need optimization
 * 
 *          The function is attributed with __attribute__((ramfunc)) to place and execute 
 *          it from RAM, ensuring faster execution.
 */
__attribute__((ramfunc))
static void timeBaseHandler(void *args) {
    timebase += TIMER_GRANULARITY_US;
    Timer_t *obj = GetQueueHead(&Tqueue); // get the first timer queued 

    while (obj) {
        Timer_t *nextObj = GetNextNode(obj); // Grab the next timer in the queue
        
        if (obj->Remaining <= TIMER_GRANULARITY_US) {
            obj->Remaining = 0;
            EventPost((Event_t *)(QueueRemove(obj))); // remove object from timers queue and post it onto events queue
        } else {
            obj->Remaining -= TIMER_GRANULARITY_US;
        }
        
        obj = nextObj; // Move to the next timer
    }
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
void Timers_Init(void) {

    // Initialization of CPU timer0
    Interrupt_register(INT_TIMER0, &cpuTimer0ISR);
    // Configure the CPU timer0 to interrupt every TICK seconds (100 microseconds)
    CPUTimer_setPeriod(CPUTIMER0_BASE, SYS_TICKS);
    // Set pre-scale counter to divide by 1 (SYSCLKOUT):
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0); 

    // Initializes timer control register. The timer is stopped, reloaded,
    // free run disabled, and interrupt enabled.
    // Additionally, the free and soft bits are set
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE, CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);           // Enable INT1.7 interrupt
    CPUTimer_startTimer(CPUTIMER0_BASE);    // starts timer

    EventInit(&cpuTimer0EventISR, timeBaseHandler, 0);
    QueueInit(&Tqueue); // initialized the queue where all timers are stored

}

void TimerInit(Timer_t * Obj, Callback_t cb, EventContext_t Context) {
    EventInit((Event_t*)Obj, cb, Context);
    Obj->Remaining = 0;
    Obj->Reload = 0;
}

void TimerStart(Timer_t * Obj, uint32_t Period) {

    if (Period < TIMER_GRANULARITY_US) {
        // if soft-timer period is less than the interrupt period we cannot ensure that the timer will trigger 
        // within the required time, instead it will be executed in the following cpu timer0 interrupt which 
        // can happen now, or anytime within the next TIMER_GRANULARITY_US. Hence, timer will not start.
        return;
    }
    Obj->Remaining = Period;
    Obj->Reload = Period;

    QueuePushToTail(&Tqueue, (void*)Obj); // add it to the timers queue
    // @TODO: make Tqueue a sorted queue in increasing order.
}

void TimerStop(Timer_t * Obj) {
    if (IsNodeInQueue(&Tqueue, Obj)) { // check if Object is part of the Timers queue
        QueueRemove(Obj); // remove it from the queue
    } else {
        // has it been posted for execution? 
    }
}

void TimerRestart(Timer_t * Obj) {
    Obj->Remaining = Obj->Reload;
    QueuePushToTail(&Tqueue, (void*)Obj); // add it to the timers queue
}
