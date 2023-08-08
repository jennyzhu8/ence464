/*
 * buttons.c
 *
 *  Created on: 9/08/2023
 *      Author: bth74
 */

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
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "button_task.h"

extern xQueueHandle g_pLEDQueue;
extern xSemaphoreHandle g_pUARTSemaphore;

// Initialises the buttons for use
void initialiseButtons(void)
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


// Button polling task, sends button info to queue if button pressed
void ButtonTask(void *pvParameters)
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
// Tests both left and right button to turn LED on and of
void Button_LED_Task(void *pvParameters)
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


