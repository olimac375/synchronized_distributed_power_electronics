// Included Files
#include "driverlib.h"
#include "device.h"
#include "inc/hw_ipc.h"

void configGPIOs(void);


void main(void)
{
    // Initialize device clock and peripherals
    Device_init();

//#define _STANDALONE

#ifdef _STANDALONE
#ifdef _FLASH
    // Send boot command to allow the CPU2 application to begin execution
    Device_bootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);
#else
    // Send boot command to allow the CPU2 application to begin execution
    Device_bootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_RAM);

#endif // _FLASH
#endif // _STANDALONE

    // Initialize GPIO and configure the GPIO pin as a push-pull output
    Device_initGPIO();
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    

    // Configure CPU2 to control the LED GPIO
    GPIO_setMasterCore(DEVICE_GPIO_PIN_LED2, GPIO_CORE_CPU2);

    SysCtl_setLowSpeedClock(SYSCTL_LSPCLK_PRESCALE_14);


    configGPIOs();

    // Hand-over the SPI modules access to CPU2
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 1, SYSCTL_CPUSEL_CPU2);// Hand-over SPI A
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 2, SYSCTL_CPUSEL_CPU2);// Hand-over SPI B


    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS0 | MEMCFG_SECT_GS1 , MEMCFG_GSRAMCONTROLLER_CPU2);

    // Sync CPU1 and CPU2. Send IPC flag 31 to CPU2.
    HWREG(IPC_BASE + IPC_O_SET) = IPC_ACK_IPC31;

    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();

    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    // Loop Forever
    for(;;)
    {
        // Turn on LED
        GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);

        // Delay for a bit.
        DEVICE_DELAY_US(500000);

        // Turn off LED
        GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);

        // Delay for a bit.
        DEVICE_DELAY_US(500000);
    }
}

void configGPIOs(void)
{
    // Initialize GPIOs used by SPIA.
    // GPIO59 - SPISOMI
    // GPIO58 - SPISIMO
    // GPIO61 - SPISTE
    // GPIO60 - SPICLK

    // GPIO59 is the SPISOMIA.                      PIN 14
    GPIO_setMasterCore(59, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_59_SPISOMIA);
    GPIO_setPadConfig(59, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(59, GPIO_QUAL_ASYNC);

    // GPIO58 is the SPISIMOA.                      PIN 15
    GPIO_setMasterCore(58, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_58_SPISIMOA);
    GPIO_setPadConfig(58, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(58, GPIO_QUAL_ASYNC);

    // GPIO61 is the SPISTEA.                       PIN 19
    GPIO_setMasterCore(61, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_61_SPISTEA);
    GPIO_setPadConfig(61, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(61, GPIO_QUAL_ASYNC);

    // GPIO60 is the SPICLKA.                       PIN 7
    GPIO_setMasterCore(60, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_60_SPICLKA);
    GPIO_setPadConfig(60, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(60, GPIO_QUAL_ASYNC);

    // Initialize GPIOs used by SPIB.
    // GPIO64 - SPISOMI
    // GPIO63 - SPISIMO
    // GPIO66 - SPISTE
    // GPIO65 - SPICLK

    // GPIO64 is the SPISOMIB.                      PIN 54
    GPIO_setMasterCore(64, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_64_SPISOMIB);
    GPIO_setPadConfig(64, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(64, GPIO_QUAL_ASYNC);

    // GPIO63 is the SPISIMOB.                      PIN 55
    GPIO_setMasterCore(63, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_63_SPISIMOB);
    GPIO_setPadConfig(63, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(63, GPIO_QUAL_ASYNC);

    // GPIO66 is the SPISTEB.                       PIN 59
    GPIO_setMasterCore(66, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_66_SPISTEB);
    GPIO_setPadConfig(66, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(66, GPIO_QUAL_ASYNC);

    // GPIO65 is the SPICLKB.                       PIN 47
    GPIO_setMasterCore(65, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_65_SPICLKB);
    GPIO_setPadConfig(65, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(65, GPIO_QUAL_ASYNC);
}

// End of File
