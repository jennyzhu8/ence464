#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"


/**
 * main.c
 */

static void NullTaskFunc(void *);
static void Blink_LED_task(void *);
static void ADC_task(void *);

int main(void)
{
    // Set the clock rate to 80 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);


    //
    // Enable the ADC0 module.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Wait for the ADC0 module to be ready.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0))
    {
    }
    //
    // Enable the first sample sequencer to capture the value of channel 0 when
    // the processor trigger occurs.
    //
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0,ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH9);
    ADCSequenceEnable(ADC0_BASE, 0);

    // For LED blinky task - initialize GPIO port F and then pin #1 (red) for output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) ; // busy-wait until GPIOF's bus clock is ready

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); // PF_1 as output
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); // PF_2 as output
    // doesn't need too much drive strength as the RGB LEDs on the TM4C123 launchpad are switched via N-type transistors

    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0); // off by default
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); // off by default

    // For read ADC task
    /*
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) ; // busy-wait until GPIOE's bus clock is ready

    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4); // PE_4 as output

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
     */

    /*
    if (pdTRUE != xTaskCreate(BlinkLED, "Blinker", 32, (void *)1, 4, NULL))
    { // (void *)1 is our pvParameters for our task func specifying PF_1
     while (1) ; // error creating task, out of memory?
    }
*/
    if (pdTRUE != xTaskCreate(Blink_LED_task, "Blinker Blue", 32, (void *)2, 4, NULL))
    { // (void *)2 is our pvParameters for our task func specifying PF_2
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(NullTaskFunc, "Null Task", 32, NULL, 4, NULL))
    {
        while(1); // Oh no! Must not have had enough memory to create task.
    }

    if (pdTRUE != xTaskCreate(ADC_task, "ADC Task", 32, (void *)4, 5, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    vTaskStartScheduler(); // Start FreeRTOS!!

    // Should never get here since the RTOS should never "exit".
    while(1);

}

//Blinky function

void Blink_LED_task(void *pvParameters) {
    /* While pvParameters is technically a pointer, a pointer is nothing
    * more than an unsigned integer of size equal to the architecture's
    * memory address bus width, which is 32-bits in ARM. We're abusing
    * the parameter then to hold a simple integer value. Could also have
    * used this as a pointer to a memory location holding the value, but
    * our method uses less memory.
    */
    const unsigned int whichLed = (unsigned int)pvParameters;
    // TivaWare GPIO calls require the pin# as a binary bitmask,
    // not a simple number. Alternately, we could have passed the
    // bitmask into pvParameters instead of a simple number.
    const uint8_t whichBit = 1 << whichLed;
    uint8_t currentValue = 0;
    while (1)
    {
        // XOR toggles the bit on/off each time this runs.
        currentValue ^= whichBit;
        GPIOPinWrite(GPIO_PORTF_BASE, whichBit, currentValue);
        // Suspend this task (so others may run) for 125ms
        // or as close as we can get with the current RTOS tick setting.
        // (vTaskDelay takes scheduler ticks as its parameter, so use the
        // pdMS_TO_TICKS macro to convert milliseconds to ticks.)
        vTaskDelay(pdMS_TO_TICKS(125));
    }
    // No way to kill this blinky task unless another task has an
    // xTaskHandle reference to it and can use vTaskDelete() to purge it.
}

static void ADC_task(void *pvParameters)
{
    uint32_t ui32Value;

    while(1)
    {
        //
        // Trigger the sample sequence.
        //
        ADCProcessorTrigger(ADC0_BASE, 0);
        //
        // Wait until the sample sequence has completed.
        //
        while(!ADCIntStatus(ADC0_BASE, 0, false))
        {
        }
        //
        // Read the value from the ADC.
        //
        ADCSequenceDataGet(ADC0_BASE, 0, &ui32Value);

        UARTprintf("HI");
    }
}
static void NullTaskFunc(void *pvParameters)
{
    while(1)
    {
        // With this task always delaying, the RTOS Idle Task runs almost
        // all the time.
        vTaskDelay(10000);
    }
}


// This is an error handling function called when FreeRTOS asserts.
// This should be used for debugging purposes
void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    (void)pcFile; // unused
    (void)ulLine; // unused
    while (true) ;
}