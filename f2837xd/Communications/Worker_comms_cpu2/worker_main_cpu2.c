// Included Files
#include <string.h>
#include "driverlib.h"
#include "device.h"
#include "Timers.h"
#include "EventsEngine.h"
#include "SystemEvents.h"

//#include "..\..\system\system.h"
#include "system.h"
#include "crc.h"


#ifndef WORKER_ID
#warning No worker ID specified, defaulting to 0.
#define WORKER_ID 0
#endif




// Defines

#define FIFO_LVL      8                     // FIFO Interrupt Level




#define DMA_TRANSFER_SIZE_TX ((MEM_BUFFER_SIZE - 1) / (16 - FIFO_LVL))
#define DMA_BURST_SIZE_TX (16 - FIFO_LVL)

#define DMA_TRANSFER_SIZE_RX ((MEM_BUFFER_SIZE - 1) / FIFO_LVL)
#define DMA_BURST_SIZE_RX (FIFO_LVL)


// DEBUG

SPI_TxFIFOLevel txFifoStatus;
SPI_RxFIFOLevel rxFifoStatus;

SPI_TxFIFOLevel txFifoStatusEndFrame;
SPI_RxFIFOLevel rxFifoStatusEndFrame;

uint16_t nFalling = 0;
uint16_t nRising = 0;
uint16_t nFrames = 0;

char interruptOrder[256];
uint16_t order_idx = 0;

uint16_t pendingTxComplete = 0;
uint16_t pendingRxComplete = 0;

Timer_t InnerLoop;




#pragma DATA_SECTION(mem_buffer, "SHARERAMGS1");  // map the RX data to memory

volatile uint16_t mem_buffer[MEM_BUFFER_SIZE];

volatile Frame * setpoints[NUM_WORKERS];
volatile Frame * measurements[NUM_WORKERS];

volatile uint16_t * currRxBuffer;
volatile uint16_t * nextRxBuffer;

volatile uint16_t * currTxBuffer;
volatile uint16_t * nextTxBuffer;

volatile uint16_t txPacketCount = 0;
volatile uint16_t rxPacketCount = 0;

void initSPIASlave(void);

__interrupt void dmaCh5ISR(void);
__interrupt void dmaCh6ISR(void);


__interrupt void spiCSISR(void);


//uint16_t* selectNextTxBuffer(void);
//uint16_t* selectNextRxBuffer(void);


/***
 *
 * BEFORE BREAK
 *
 * CRC function checking
 *
 *
 *
 *
 */


void main(void)
{



    // Initialize device clock and peripherals
     Device_init();

//
    currRxBuffer = mem_buffer + CHUNK_SIZE;
    nextRxBuffer = mem_buffer + CHUNK_SIZE;

    currTxBuffer = mem_buffer;
    nextTxBuffer = mem_buffer;


    // Compute pointers to chunks of memory, depending on own WORKER_ID
    int worker;
    for (worker = 0; worker < NUM_WORKERS; worker++) {
        setpoints[worker] = (Frame *)(mem_buffer + (NUM_WORKERS - 1 - worker + (WORKER_ID + 1)) * CHUNK_SIZE);
        measurements[worker] = (Frame*)(mem_buffer + ((2 * NUM_WORKERS - 1 - worker + (WORKER_ID + 1)) % (2 * NUM_WORKERS)) * CHUNK_SIZE);

    }

    // Populate some dummy values
    uint16_t i;
    for (i = 0; i < CHUNK_SIZE; i++) {
        mem_buffer[i] = (100 + WORKER_ID) * 100 + i;
    }

    // Initialize CRC LUT
    crcInit();

    // Initialize GPIO and configure the GPIO pin as a push-pull output
    // This is configured by CPU1

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();

    Timers_Init();
    EventsEngineInit();

    // Wait until CPU01 is ready and IPC flag 31 is set
    while(!(HWREG(IPC_BASE + IPC_O_STS) & (IPC_ACK_IPC31))) { }

    // ACK IPC flag 31 for CPU1
    HWREG(IPC_BASE + IPC_O_ACK) = IPC_ACK_IPC31;

    {   // DMA

        //Initialize DMA
        DMA_initController();

        // configure DMA CH5 for TX
        DMA_configAddresses(DMA_CH5_BASE, (const void *)(SPIA_BASE + SPI_O_TXBUF), (const void *)mem_buffer);
        DMA_configBurst(DMA_CH5_BASE,DMA_BURST_SIZE_TX,1,0);
        DMA_configTransfer(DMA_CH5_BASE,DMA_TRANSFER_SIZE_TX,1,0);
        DMA_configMode(DMA_CH5_BASE,    DMA_TRIGGER_SPIATX, 
                                        DMA_CFG_ONESHOT_DISABLE     |
                                        DMA_CFG_CONTINUOUS_DISABLE   |
                                        DMA_CFG_SIZE_16BIT);

        DMA_setInterruptMode(DMA_CH5_BASE,DMA_INT_AT_END);
        DMA_enableTrigger(DMA_CH5_BASE);
        DMA_enableInterrupt(DMA_CH5_BASE);
        DMA_disableOverrunInterrupt(DMA_CH5_BASE);

        

        // configure DMA CH6 for RX

        DMA_configAddresses(DMA_CH6_BASE, (const void *)mem_buffer + CHUNK_SIZE, (const void *)(SPIA_BASE + SPI_O_RXBUF));
        DMA_configBurst(DMA_CH6_BASE,DMA_BURST_SIZE_RX,0,1);
        DMA_configTransfer(DMA_CH6_BASE,DMA_TRANSFER_SIZE_RX,0,1);
        DMA_configMode(DMA_CH6_BASE,    DMA_TRIGGER_SPIARX, 
                                        DMA_CFG_ONESHOT_DISABLE    |
                                        DMA_CFG_CONTINUOUS_DISABLE  |
                                        DMA_CFG_SIZE_16BIT);

        DMA_setInterruptMode(DMA_CH6_BASE,DMA_INT_AT_END);
        DMA_enableTrigger(DMA_CH6_BASE);
        DMA_enableInterrupt(DMA_CH6_BASE);
        DMA_disableOverrunInterrupt(DMA_CH6_BASE);

        

        // Ensure DMA is connected to Peripheral Frame 2 bridge (EALLOW protected)
        SysCtl_selectSecMaster(SYSCTL_SEC_MASTER_DMA, SYSCTL_SEC_MASTER_DMA);
    }

    {   // SPI
        // Set up SPI A as slave, initializing it for FIFO mode

        initSPIASlave();

        // Enable interrupts required for this example
        Interrupt_enable(INT_DMA_CH5);
        Interrupt_enable(INT_DMA_CH6);

        Interrupt_register(INT_DMA_CH5, &dmaCh5ISR);
        Interrupt_register(INT_DMA_CH6, &dmaCh6ISR);

//        DMA_startChannel(DMA_CH5_BASE);
//        DMA_startChannel(DMA_CH6_BASE);
    }

    GPIO_setInterruptType(GPIO_INT_XINT1, GPIO_INT_TYPE_BOTH_EDGES);

    GPIO_enableInterrupt(GPIO_INT_XINT1);

    Interrupt_register(INT_XINT1, &spiCSISR);
    Interrupt_enable(INT_XINT1);


    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    TimerInit(&InnerLoop, InnerLoop_Handler, 0);    // initializing timer and timer handler
    TimerStart(&InnerLoop, SECONDS_TO_TICKS(0.5f));      // Start timer


    txFifoStatus = SPI_getTxFIFOStatus(SPIA_BASE);
    rxFifoStatus = SPI_getRxFIFOStatus(SPIA_BASE);


    // Loop forever; processing pending events.
    EventsEngine();
}

void InnerLoop_Handler(void * args) {
    GPIO_togglePin(DEVICE_GPIO_PIN_LED2);

    // compute CRC for own data, save in first word
//    mem_buffer[0] = crcFast((mem_buffer + 1), (CHUNK_SIZE - 1));

    measurements[WORKER_ID]->hdr.crc = crcFast(AFTER_CRC(measurements[WORKER_ID]), sizeof(Frame) - sizeof(crc_t));

    TimerRestart((Timer_t *)args);
}

// Function to configure SPI A as slave with FIFO enabled.
void initSPIASlave(void)
{
    // Must put SPI into reset before configuring it
    SPI_disableModule(SPIA_BASE);

    // SPI configuration. Use a 500kHz SPICLK and 16-bit word size.
    SPI_setConfig(SPIA_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0, SPI_MODE_SLAVE,  DEVICE_LSPCLK_FREQ / 128, 16);
//    SPI_enableHighSpeedMode(SPIA_BASE);
    SPI_disableLoopback(SPIA_BASE);
    SPI_setEmulationMode(SPIA_BASE, SPI_EMULATION_FREE_RUN);

    // FIFO and interrupt configuration
    SPI_enableFIFO(SPIA_BASE);
    SPI_clearInterruptStatus(SPIA_BASE, SPI_INT_RXFF | SPI_INT_TXFF);
    SPI_setFIFOInterruptLevel(SPIA_BASE, (SPI_TxFIFOLevel) FIFO_LVL, (SPI_RxFIFOLevel)FIFO_LVL);

//    SPI_enableInterrupt(SPIA_BASE, SPI_INT_TXFF);
//    SPI_enableInterrupt(SPIA_BASE, SPI_INT_RXFF);

    // Configuration complete. Enable the module.
    SPI_enableModule(SPIA_BASE);

}





// TX Interrupt
// Interrupts when Transfer from RAM to SPI TX FIFO completes

__interrupt void dmaCh5ISR(void) {
    EALLOW;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
    EDIS;

    interruptOrder[order_idx++] = 't';if (order_idx > 255) order_idx = 0;
    pendingTxComplete = 0;

//    SPI_clearInterruptStatus(SPIA_BASE, SPI_INT_TXFF);

//    txFifoStatus = SPI_getTxFIFOStatus(SPIA_BASE);
//    rxFifoStatus = SPI_getRxFIFOStatus(SPIA_BASE);

    // Update TX Buffer Source
//    currTxBuffer = nextTxBuffer;
//    nextTxBuffer = selectNextTxBuffer();

//       DMA_stopChannel(DMA_CH5_BASE);
//       DMA_configAddresses(DMA_CH5_BASE, (const void *)(SPIA_BASE + SPI_O_TXBUF), (const void *)nextTxBuffer);
//       DMA_startChannel(DMA_CH5_BASE);



}

// RX Interrupt
// Interrupts when Transfer from SPI RX FIFO to RAM completes
__interrupt void dmaCh6ISR(void) {

    EALLOW;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
    EDIS;

    interruptOrder[order_idx++] = 'r';if (order_idx > 255) order_idx = 0;
    pendingRxComplete = 0;

    crc_t crc_check;

    int i;

    // CHeck all received CRCs
    for (i = 0; i < NUM_WORKERS; i++) {
        // Recompute CRC
        crc_check = crcFast(AFTER_CRC(setpoints[i]), sizeof(Frame) - sizeof(crc_t));

        // Check agaist recieved CRC
        if (crc_check != setpoints[i]->hdr.crc) {
            interruptOrder[order_idx++] = 'C';if (order_idx > 255) order_idx = 0;
            interruptOrder[order_idx++] = 'S';if (order_idx > 255) order_idx = 0;
            interruptOrder[order_idx++] = '0' + i;if (order_idx > 255) order_idx = 0;
        }

        if (i == WORKER_ID) continue;

        crc_check = crcFast(AFTER_CRC(measurements[i]), sizeof(Frame) - sizeof(crc_t));
        // Check agaist recieved CRC
        if (crc_check != measurements[i]->hdr.crc) {
            interruptOrder[order_idx++] = 'C';if (order_idx > 255) order_idx = 0;
            interruptOrder[order_idx++] = 'M';if (order_idx > 255) order_idx = 0;
            interruptOrder[order_idx++] = '0' + i;if (order_idx > 255) order_idx = 0;
        }



    }

    // Update DMA RX Destination

//    currRxBuffer = nextRxBuffer;
//    nextRxBuffer = selectNextRxBuffer();
//
//    DMA_configAddresses(DMA_CH6_BASE, (const void *)nextRxBuffer, (const void *)(SPIA_BASE + SPI_O_RXBUF));


}


uint16_t txPacketEnd;
uint16_t rxPacketEnd;

uint16_t txPacketStart;
uint16_t rxPacketStart;


__interrupt void spiCSISR(void) {

    static int lastEdge = 1;

    // If Pin is low (FALLING EDGE)
    if (GPIO_readPin(22) == 0) {
        if (lastEdge == 1) { // Debouncing
            lastEdge = 0;

            nFalling++;
            interruptOrder[order_idx++] = '\\';if (order_idx > 255) order_idx = 0;

            txPacketStart = txPacketCount;
            rxPacketStart = rxPacketCount;

            if (pendingTxComplete || pendingRxComplete) {
                interruptOrder[order_idx++] = 'X';if (order_idx > 255) order_idx = 0;
            }

            // Start transfer at CS falling
            DMA_startChannel(DMA_CH5_BASE);
            DMA_startChannel(DMA_CH6_BASE);

        }

    } else { // RISING EDGE, Frame transfer complete
        if (lastEdge == 0) {
            lastEdge = 1;

            txPacketEnd = txPacketCount;
            rxPacketEnd = rxPacketCount;

            // Set TX address back to control_output
            // Allow RX to continue until its completed transferring its data.
            // DONT DO ANYTHING HERE ?
            interruptOrder[order_idx++] = '/';if (order_idx > 255) order_idx = 0;
            nRising++;
            nFrames++;

            // CHECK LAST RX BUFFER HAS VALID ID

            // Check packet count it right
//
//            if (txPacketEnd != 2 * NUM_WORKERS) {
//                txPacketEnd = 1;
//                interruptOrder[order_idx++] = 'E';if (order_idx > 255) order_idx = 0;
//
//                interruptOrder[order_idx++] = 't';if (order_idx > 255) order_idx = 0;
//
//            }
//
//            if (rxPacketEnd != 2 * NUM_WORKERS - 1) {
//                rxPacketEnd = 0;
//                interruptOrder[order_idx++] = 'E';if (order_idx > 255) order_idx = 0;
//                interruptOrder[order_idx++] = 'r';if (order_idx > 255) order_idx = 0;
//            }
//
//            txPacketCount = 1;
//            rxPacketCount = 0;



        }

    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

}



//uint16_t* selectNextTxBuffer(void) {
//
//    uint16_t * next;
//
//    if (txPacketCount >= 2 * NUM_WORKERS - 1) { // Wrap around, set back to local_measurements
//        next = mem_buffer;
////        interruptOrder[order_idx++] = '0';if (order_idx > 255) order_idx = 0;
//    } else {
//        next = mem_buffer + txPacketCount * CHUNK_SIZE;
//    }
//
////    txPacketCount = (txPacketCount + 1) % (2 * NUM_WORKERS);
//    txPacketCount++;
//
//    return (uint16_t *)next;
//
//}
//
//uint16_t* selectNextRxBuffer(void) {
//
//    uint16_t * next;
//
//    rxPacketCount++;
////    rxPacketCount = (rxPacketCount + 1) % (2 * NUM_WORKERS);
//
//    if (rxPacketCount > 2 * NUM_WORKERS - 1) {
//        next = mem_buffer + (2 * NUM_WORKERS - 1) * CHUNK_SIZE; // if packet too long over lap last
////      interruptOrder[order_idx++] = '0';if (order_idx > 255) order_idx = 0;
//    }
//    else {
//        next = mem_buffer + (rxPacketCount + 1) * CHUNK_SIZE;
//    }
//
//    return (uint16_t * )next;
//
//}




// End of File
