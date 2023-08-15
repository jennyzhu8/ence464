/**
 * main.c
 */

#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "driverlib/rom.h"
#include "driverlib/uart.h"
#include "drivers/buttons.h"
#include "queue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "priorities.h"
#include "semphr.h"
#include "PWM.h"
#include "LED.h"
#include "readADC_task.h"
#include "button_task.h"
#include "PID_task.h"


#define BUF_SIZE 5
#define LED_ITEM_SIZE           sizeof(uint8_t)
#define LED_QUEUE_SIZE          5
#define ALT_ITEM_SIZE           sizeof(uint8_t)
#define ALT_QUEUE_SIZE          5
#define TARGET_ITEM_SIZE        sizeof(uint8_t)
#define TARGET_QUEUE_SIZE       5

/* QUEUES
 * ----------------------------------------------------------------------------------------------------*/
// Queue for the output to the LED
xQueueHandle g_pLEDQueue;
// Queue for the output to the PWM
xQueueHandle g_pALTQueue;
//Queue for the buttons to change the target height
xQueueHandle g_pTARGETQueue;

/* SEMAPHORES
 * ----------------------------------------------------------------------------------------------------*/
// Mutex for protecting against concurrent access of UART from multiple tasks
xSemaphoreHandle g_pUARTSemaphore;

//Function definitions
static void NullTaskFunc(void *);
void ConfigureUART(void);


void
ConfigureUART(void)
{
    // Enable the GPIO Peripheral used by the UART.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART mode.
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 115200, 16000000);
}

int main(void)
{
    // Set the clock rate to 80 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Initilisation of task drivers
    ConfigureUART();
    initialisePWM();
    control_init();
    initialiseLED();//DELETE
    initialiseADC();
    initialiseButtons();

    // Starts the PWM
    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, true);

    // Creates the queues for target height, target yaw and altitude values
    g_pLEDQueue = xQueueCreate(LED_QUEUE_SIZE, LED_ITEM_SIZE);
    g_pALTQueue = xQueueCreate(ALT_QUEUE_SIZE, ALT_ITEM_SIZE);
    g_pTARGETQueue = xQueueCreate(TARGET_QUEUE_SIZE, TARGET_ITEM_SIZE);

    // Create a mutex to guard the UART, Height and Yaw.
    g_pUARTSemaphore = xSemaphoreCreateMutex();

    // Creates FreeRTOS Tasks
    if (pdTRUE != xTaskCreate(NullTaskFunc, "Null Task", 32, NULL, 4, NULL))
    {
        while(1); // Oh no! Must not have had enough memory to create task.
    }

    if (pdTRUE != xTaskCreate(altitude_ADC_task, "ADC Task", 32, (void *)4, 3, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(ButtonTask, (const portCHAR *)"Button Task", 128, NULL, 1, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(PID_Task, "Get PWM Task", 128, NULL, 2, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    vTaskStartScheduler(); // Start FreeRTOS!!

    // Should never get here since the RTOS should never "exit".
    while(1);

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
