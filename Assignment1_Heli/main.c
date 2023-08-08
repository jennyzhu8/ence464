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


#define BUF_SIZE 5
#define LED_ITEM_SIZE           sizeof(uint8_t)
#define LED_QUEUE_SIZE          5
#define PWM_ITEM_SIZE           sizeof(uint8_t)
#define PWM_QUEUE_SIZE          5

xQueueHandle g_pLEDQueue;
xQueueHandle g_pPWMQueue;

//*****************************************************************************
//
// The mutex that protects concurrent access of UART from multiple tasks.
//
//*****************************************************************************
xSemaphoreHandle g_pUARTSemaphore;

// PWM configuration
#define PWM_START_RATE_HZ  250

// Buttons (from buttons4.h which we should use instead of buttons.h)
// UP button
#define UP_BUT_PERIPH  SYSCTL_PERIPH_GPIOE
#define UP_BUT_PORT_BASE  GPIO_PORTE_BASE
#define UP_BUT_PIN  GPIO_PIN_0
#define UP_BUT_NORMAL  false
// DOWN button
#define DOWN_BUT_PERIPH  SYSCTL_PERIPH_GPIOD
#define DOWN_BUT_PORT_BASE  GPIO_PORTD_BASE
#define DOWN_BUT_PIN  GPIO_PIN_2
#define DOWN_BUT_NORMAL  false

static void NullTaskFunc(void *);
static void ADC_task(void *);
static void Button_LED_Task(void *);
static void ButtonTask(void *);
static void Get_PWM_Task(void *);


void initialiseADC (void);
void initialiseButtons (void);

void
ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Use the internal 16MHz oscillator as the UART clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);
}



void initialiseADC (void)
{
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
}

// Initialises the buttons for use
void
initialiseButtons(void)
{
    //
    // Unlock the GPIO LOCK register for Right button to work.
    //
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0xFF;

//    // UP button (active HIGH)
//    SysCtlPeripheralEnable (UP_BUT_PERIPH);
//    GPIOPinTypeGPIOInput (UP_BUT_PORT_BASE, UP_BUT_PIN);
//    GPIOPadConfigSet (UP_BUT_PORT_BASE, UP_BUT_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
//    but_normal[UP] = UP_BUT_NORMAL;
//    // DOWN button (active HIGH)
//    SysCtlPeripheralEnable (DOWN_BUT_PERIPH);
//    GPIOPinTypeGPIOInput (DOWN_BUT_PORT_BASE, DOWN_BUT_PIN);
//    GPIOPadConfigSet (DOWN_BUT_PORT_BASE, DOWN_BUT_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    // Initialize the buttons
    //
    ButtonsInit();
}


int main(void)
{

    // Set the clock rate to 80 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    ConfigureUART();
    initialisePWM();
    initialiseLED();
    initialiseADC();
    initialiseButtons();

    PWMOutputState(PWM_MAIN_BASE, PWM_MAIN_OUTBIT, true);

    //delete later if an issue : initialising the queue
    g_pLEDQueue = xQueueCreate(LED_QUEUE_SIZE, LED_ITEM_SIZE);
    g_pPWMQueue = xQueueCreate(PWM_QUEUE_SIZE, PWM_ITEM_SIZE);

    //
    // Create a mutex to guard the UART.
    //
    g_pUARTSemaphore = xSemaphoreCreateMutex();


//    if (pdTRUE != xTaskCreate(Blink_LED_task, "Blinker Blue", 32, (void *)2, 4, NULL))
//    { // (void *)2 is our pvParameters for our task func specifying PF_2
//         while (1) ; // error creating task, out of memory?
//    }

    if (pdTRUE != xTaskCreate(NullTaskFunc, "Null Task", 32, NULL, 4, NULL))
    {
        while(1); // Oh no! Must not have had enough memory to create task.
    }

    if (pdTRUE != xTaskCreate(ADC_task, "ADC Task", 32, (void *)4, 2, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(ButtonTask, (const portCHAR *)"Button Task", 128, NULL, 1, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }

    if (pdTRUE != xTaskCreate(Button_LED_Task, "Button LED Task", 32, (void *)2, 2, NULL))
    {
         while (1) ; // error creating task, out of memory?
    }
    if (pdTRUE != xTaskCreate(Get_PWM_Task, "Get PWM Task", 128, NULL, 2, NULL))
        {
             while (1) ; // error creating task, out of memory?
        }

    vTaskStartScheduler(); // Start FreeRTOS!!

    // Should never get here since the RTOS should never "exit".
    while(1);

}


static void ADC_task(void *pvParameters)
{
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint16_t i;
    uint32_t ui32Value;
    uint32_t alt_buffer[BUF_SIZE];
    uint8_t index = 0;
    int32_t alt = 0;
    uint32_t height;
    uint32_t count;
    uint8_t message;
    uint32_t max_voltage;
    uint32_t min_voltage;
    uint32_t voltage_difference = 1340;

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
    max_voltage = ui32Value;
    min_voltage = max_voltage -voltage_difference;


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
        alt_buffer[index] = ui32Value;
        index = (index + 1) % BUF_SIZE;
        alt = 0;
        count = 0;
        for (i=0; i < BUF_SIZE; i++) {
            if (alt_buffer[i] != 0){
                alt = alt + alt_buffer[i];
                count++;
            }
        }
        alt = alt / count;
        // Values depend on the rig you are using
        // Min Altitude = 2230, Max Altitude = 1000
        // For emulator to 100%
        // Min Alt = 3030, Max alt = 1710
        // Emulator to max which is 133%
        // Min alt =3030, max alt = 1330
        // PWM hover is 53%

        //UARTprintf("Raw = %d\n", alt);
        //getting height between 1710 and 0
        height = alt-min_voltage;
        //getting height as a percentage
        height =  (100 - ((height*100)/voltage_difference));
        message = height;

        if(xQueueSend(g_pLEDQueue, &message, portMAX_DELAY) != pdPASS)
        {
            //
            // Error. The queue should never be full. If so print the
            // error message on UART and wait for ever.
            //
            UARTprintf("\nQueue full. This should never happen.\n");
            while(1)
            {
            }
        }
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
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

// Button polling task, sends button info to queue if button pressed
static void
ButtonTask(void *pvParameters)
{
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint8_t ui8CurButtonState, ui8PrevButtonState;
    uint8_t ui8Message;

    ui8CurButtonState = ui8PrevButtonState = 0;

    //
    // Get the current tick count.
    //
    ui16LastTime = xTaskGetTickCount();

    while(1)
    {
        //
        // Poll the debounced state of the buttons.
        //
        ui8CurButtonState = ButtonsPoll(0, 0);

        //
        // Check if previous debounced state is equal to the current state.
        //
        if(ui8CurButtonState != ui8PrevButtonState)
        {
            ui8PrevButtonState = ui8CurButtonState;

            //
            // Check to make sure the change in state is due to button press
            // and not due to button release.
            //
            if((ui8CurButtonState & ALL_BUTTONS) != 0)
            {
                if((ui8CurButtonState & ALL_BUTTONS) == LEFT_BUTTON)
                {
                    ui8Message = LEFT_BUTTON;
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Left Button is pressed.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
                }
                else if((ui8CurButtonState & ALL_BUTTONS) == RIGHT_BUTTON)
                {
                    ui8Message = RIGHT_BUTTON;
                    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
                    UARTprintf("Right Button is pressed.\n");
                    xSemaphoreGive(g_pUARTSemaphore);
                }

                //
                // Pass the value of the button pressed to Button_LED_Task.
                //
                if(xQueueSend(g_pLEDQueue, &ui8Message, portMAX_DELAY) !=
                   pdPASS)
                {
                    //
                    // Error. The queue should never be full. If so print the
                    // error message on UART and wait for ever.
                    //
                    UARTprintf("\nQueue full. This should never happen.\n");
                    while(1)
                    {
                    }
                }
            }
        }

        //
        // Wait for the required amount of time to check back.
        //
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }
}

// For testing the receiving of the button from the queue.
// Tests both left and right button to turn LED on and off.
static void Button_LED_Task(void *pvParameters)
{
    const unsigned int whichLed = (unsigned int)pvParameters;
    const uint8_t whichBit = 1 << whichLed;
    uint8_t currentValue = 0;
    uint8_t i8Message;

    while(1)
    {
        if(xQueueReceive(g_pLEDQueue, &i8Message, pdMS_TO_TICKS(125)) == pdPASS) // ticks to wait must be > 0 so the task doesn't get stuck here
        {
            if(i8Message == LEFT_BUTTON)
            {
                //
                // Update the LED to turn on.
                //
                currentValue = whichBit;
                GPIOPinWrite(GPIO_PORTF_BASE, whichBit, currentValue);
                vTaskDelay(pdMS_TO_TICKS(125));
            }
            if(i8Message == RIGHT_BUTTON)
            {
                //
                // Update the LED to turn off.
                //
                currentValue = 0;
                GPIOPinWrite(GPIO_PORTF_BASE, whichBit, currentValue);
                vTaskDelay(pdMS_TO_TICKS(125));
            }
        }
    }
}

static void Get_PWM_Task(void *pvParameters)
{

    uint8_t i8Message;
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint8_t target_height = 100;
    uint8_t height;
    uint32_t pwm;
    while(1)
    {
        if(xQueueReceive(g_pLEDQueue, &i8Message, pdMS_TO_TICKS(125)) == pdPASS) // ticks to wait must be > 0 so the task doesn't get stuck here
        {
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Altitude = %d\n", i8Message);
            xSemaphoreGive(g_pUARTSemaphore);
            vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
        }
        height = i8Message;
        pwm = 53 + (target_height - height);
        if (pwm > 80)
        {
            pwm = 80;
        }

        setPWM (PWM_START_RATE_HZ, pwm);
    }
}


// This is an error handling function called when FreeRTOS asserts.
// This should be used for debugging purposes
void vAssertCalled( const char * pcFile, unsigned long ulLine ) {
    (void)pcFile; // unused
    (void)ulLine; // unused
    while (true) ;
}


