// Included Files
#include <string.h>
#include "driverlib.h"
#include "device.h"
#include "Timers.h"
#include "EventsEngine.h"
#include "SystemEvents.h"

#include "system.h"
#include "crc.h"


//#define NUM_WORKERS 2
//#define CHUNK_SIZE 32

//#define MEM_BUFFER_SIZE (2 * NUM_WORKERS * CHUNK_SIZE)

#define FIFO_LVL      8                     // FIFO Interrupt Level


#define DMA_TRANSFER_SIZE_TX ((MEM_BUFFER_SIZE - 1) / (16 - FIFO_LVL))
#define DMA_BURST_SIZE_TX (16 - FIFO_LVL)

#define DMA_TRANSFER_SIZE_RX ((NUM_WORKERS * CHUNK_SIZE) / FIFO_LVL)
#define DMA_BURST_SIZE_RX (FIFO_LVL)



#pragma DATA_SECTION(mem_buffer, "SHARERAMGS1");  // map the RX data to memory

volatile uint16_t mem_buffer[MEM_BUFFER_SIZE];

volatile Frame * setpoints[NUM_WORKERS];
volatile Frame * measurements[NUM_WORKERS];


volatile uint16_t dma5_count = 0;
volatile uint16_t dma6_count = 0;

volatile uint16_t packet_count = 0;

char interruptOrder[256];
uint16_t order_idx = 0;

Timer_t InnerLoop;

void initSPIAMaster(void);
void initSPIBSlave(void);

__interrupt void dmaCh5ISR(void);
__interrupt void dmaCh6ISR(void);


void main(void)
{




    // Initialize device clock and peripherals
    Device_init();

    // Initialize GPIO and configure the GPIO pin as a push-pull output
    // This is configured by CPU1

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();

    Timers_Init();
    EventsEngineInit();


    // Compute pointers to chunks of memory
    int worker;
    for (worker = 0; worker < NUM_WORKERS; worker++) {
        setpoints[worker] = (Frame *)(mem_buffer + (NUM_WORKERS - 1 - worker) * CHUNK_SIZE);
        measurements[worker] = (Frame *)(mem_buffer + (2 * NUM_WORKERS - 1 - worker) * CHUNK_SIZE);
    }


    // Fill SETPOINTS with some dummy data
    uint16_t i,j;
    for (i = 0; i < NUM_WORKERS; i++) {
        for (j = 0; j < CHUNK_SIZE; j++) {
             mem_buffer[(NUM_WORKERS - 1 - i) * CHUNK_SIZE + j] = (i+1) * 1000 + j;
        }

    }

    // Initialize CRC LUT
    crcInit();


  for (i = 0; i < NUM_WORKERS; i++) {
      setpoints[i]->hdr.crc = crcFast(AFTER_CRC(setpoints[i]), sizeof(Frame) - sizeof(crc_t));
  }



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
                                        DMA_CFG_CONTINUOUS_DISABLE  |
                                        DMA_CFG_SIZE_16BIT);

        DMA_setInterruptMode(DMA_CH5_BASE,DMA_INT_AT_END);
        DMA_enableTrigger(DMA_CH5_BASE);
        DMA_enableInterrupt(DMA_CH5_BASE);
        DMA_disableOverrunInterrupt(DMA_CH5_BASE);

        

        // configure DMA CH6 for RX

        DMA_configAddresses(DMA_CH6_BASE, (const void *)mem_buffer + NUM_WORKERS * CHUNK_SIZE, (const void *)(SPIB_BASE + SPI_O_RXBUF));
        DMA_configBurst(DMA_CH6_BASE,DMA_BURST_SIZE_RX,0,1);
        DMA_configTransfer(DMA_CH6_BASE,DMA_TRANSFER_SIZE_RX,0,1);
        DMA_configMode(DMA_CH6_BASE,    DMA_TRIGGER_SPIBRX,
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

        // Set up SPI A as master, initializing it for FIFO mode
        initSPIAMaster();

        // Set up SPI B as slave, initializing it for FIFO mode
        initSPIBSlave();

        // Enable interrupts required for this example
        Interrupt_enable(INT_DMA_CH5);
        Interrupt_enable(INT_DMA_CH6);

        Interrupt_register(INT_DMA_CH5, &dmaCh5ISR);
        Interrupt_register(INT_DMA_CH6, &dmaCh6ISR);

        DMA_startChannel(DMA_CH5_BASE);
        DMA_startChannel(DMA_CH6_BASE);
    }

    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    TimerInit(&InnerLoop, InnerLoop_Handler, 0);    // initializing timer and timer handler
    TimerStart(&InnerLoop, SECONDS_TO_TICKS(0.1f));      // Start timer

//    memset((void *)&master_sData, dma5_count, MEM_BUFFER_SIZE );
//    memset((void *)&master_rData, 0, MEM_BUFFER_SIZE );


    // Loop forever; processing pending events.
    EventsEngine();
}



void InnerLoop_Handler(void * args) {
    GPIO_togglePin(DEVICE_GPIO_PIN_LED2);

//    SPI_clearInterruptStatus(SPIA_BASE, SPI_INT_RXFF);
//    SPI_clearInterruptStatus(SPIA_BASE, SPI_INT_TXFF);
    

    // DEBUG
    if (++packet_count % 20 == 0) {
        interruptOrder[order_idx++] = 'B';if (order_idx > 255) order_idx = 0;
    }

    // Compute CRC16 for each setpoint chunk
    int i;
    for (i = 0; i < NUM_WORKERS; i++) {
        setpoints[i]->hdr.crc = crcFast(AFTER_CRC(setpoints[i]), sizeof(Frame) - sizeof(crc_t));
    }


    DMA_startChannel(DMA_CH5_BASE);

    SPI_enableModule(SPIB_BASE);
    DMA_startChannel(DMA_CH6_BASE);

    TimerRestart((Timer_t *)args);
}

// Function to configure SPI A as slave with FIFO enabled.
void initSPIAMaster(void)
{
    // Must put SPI into reset before configuring it
    SPI_disableModule(SPIA_BASE);

    // SPI configuration. Use a 500kHz SPICLK and 16-bit word size.
    SPI_setConfig(SPIA_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0, SPI_MODE_MASTER, DEVICE_LSPCLK_FREQ / 128, 16);
//    SPI_enableHighSpeedMode(SPIA_BASE);
    SPI_disableLoopback(SPIA_BASE);
    SPI_setEmulationMode(SPIA_BASE, SPI_EMULATION_FREE_RUN);

    // FIFO and interrupt configuration
    SPI_enableFIFO(SPIA_BASE);
    SPI_clearInterruptStatus(SPIA_BASE, SPI_INT_RXFF | SPI_INT_TXFF);
    SPI_setFIFOInterruptLevel(SPIA_BASE, (SPI_TxFIFOLevel) FIFO_LVL, (SPI_RxFIFOLevel)FIFO_LVL);
    // SPI_enableInterrupt(SPIA_BASE, SPI_INT_RXFF);

    // Configuration complete. Enable the module.
    SPI_enableModule(SPIA_BASE);
}

// Function to configure SPI A as slave with FIFO enabled.
void initSPIBSlave(void)
{
    // Must put SPI into reset before configuring it
    SPI_disableModule(SPIB_BASE);

    // SPI configuration. Use a 500kHz SPICLK and 16-bit word size.
    SPI_setConfig(SPIB_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0, SPI_MODE_SLAVE,  DEVICE_LSPCLK_FREQ / 128, 16);
//    SPI_enableHighSpeedMode(SPIB_BASE);
    SPI_disableLoopback(SPIB_BASE);
    SPI_setEmulationMode(SPIB_BASE, SPI_EMULATION_FREE_RUN);

    // FIFO and interrupt configuration
    SPI_enableFIFO(SPIB_BASE);
    SPI_clearInterruptStatus(SPIB_BASE, SPI_INT_RXFF | SPI_INT_TXFF);
    SPI_setFIFOInterruptLevel(SPIB_BASE, (SPI_TxFIFOLevel) FIFO_LVL, (SPI_RxFIFOLevel)FIFO_LVL);

//    SPI_enableInterrupt(SPIB_BASE, SPI_INT_TXFF);
//    SPI_enableInterrupt(SPIB_BASE, SPI_INT_RXFF);

    // Configuration complete. Enable the module.
    SPI_enableModule(SPIB_BASE);
}

// TX Interrupt

__interrupt void dmaCh5ISR(void) {
    EALLOW;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
    EDIS;

    interruptOrder[order_idx++] = 't';if (order_idx > 255) order_idx = 0; // DEBUG


    return;
}

// RX Interrupt

__interrupt void dmaCh6ISR(void) {

    EALLOW;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP7);
    EDIS;

    // When buffer filled, stop receving for frame.
    SPI_disableModule(SPIB_BASE);
    SPI_resetRxFIFO(SPIB_BASE);

    interruptOrder[order_idx++] = 'r';if (order_idx > 255) order_idx = 0; // DEBUG

    dma6_count++;

    // Recompute CRC
    crc_t crc_check = crcFast(AFTER_CRC(measurements[0]), sizeof(Frame) - sizeof(crc_t));

    if (crc_check != measurements[0]->hdr.crc) {
        interruptOrder[order_idx++] = 'C';if (order_idx > 255) order_idx = 0;
    }


    return;
}

// End of File
