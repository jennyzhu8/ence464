/*
 * Control_task.c
 *
 *  Created on: 17/08/2023
 *      Author: bth74, hjo48
 */

#include <FreeRTOS.h>
#include <PID_controller.h>
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

#include "queue.h"
#include "FreeRTOS.h"
#include "task.h"
#include "priorities.h"
#include "semphr.h"
#include "PWM.h"
#include "button_task.h"
#include "PID_controller.h"
#include "control_task.h"

#define CONTROL_DT (1.f/50.f)

// *******************************************************
// Global PID struct variables for the different Motors
// *******************************************************
PID main_rotor;
//PID tail;


// *******************************************************
// Global Mutex and Queue external definitions
// *******************************************************
extern xSemaphoreHandle g_pUARTSemaphore;
extern xQueueHandle g_pALTQueue;
extern xQueueHandle g_pTARGETQueue;


// *******************************************************
// Control_Task: Task function called by FreeRTOS. Updates the controllers to the motors by
//  checking for button presses and then updating the target heights inputed into the PID controllers.
void
Control_Task(void *pvParameters)
{
    uint8_t button;
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    int8_t target_height = 50;
    uint8_t height;
    int16_t height_error;

    while(1)
    {
        //getting the target height from the buttons
        if(xQueueReceive(g_pTARGETQueue, &button, pdMS_TO_TICKS(125)) == pdPASS)
        {
            if(button == DOWN)
            {
                // Update the target height
                target_height = target_height - STEP_SIZE_HEIGHT;
                if (target_height < 0) {
                    target_height = 0;
                }
            }
            if(button == UP)
            {
                // Update the target height
                target_height = target_height + STEP_SIZE_HEIGHT;
                if (target_height > 100) {
                    target_height = 100;
                }
            }
        }

        //prints the Target height
        xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
        UARTprintf("Target = %d     ", target_height);
        xSemaphoreGive(g_pUARTSemaphore);

        if(xQueueReceive(g_pALTQueue, &height, pdMS_TO_TICKS(125)) == pdPASS) // ticks to wait must be > 0 so the task doesn't get stuck here
        {
            //prints the Current Altitude
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Altitude = %d    ", height);
            xSemaphoreGive(g_pUARTSemaphore);

            vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);

            //sets and inputs the height error
            height_error = target_height - height;
            control_update(height_error, 0);

            //prints the Current Height Error
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Error = %d    ", height_error);
            xSemaphoreGive(g_pUARTSemaphore);
        }

    }
}

// *******************************************************
// initialiseControl: Inititialiser for the control task. This involves initialising
//  each PID controller for the system.
void
initialiseControl(void) {
    pid_init(&main_rotor, 1.0, 0, 1.0, 100.0);
    //pid_init(&tail, 0.2f, 2.0f, 0.0f, 300.0f);
}

// *******************************************************
// control_update: Updates the PID controllers and then sets the PWM signals for the motors
void
control_update(int16_t alt_error, int16_t yaw_error) {
    int16_t pwm;
    int16_t raw;

    pid_update(&main_rotor, alt_error, CONTROL_DT);

    pwm = PWM_MID_VALUE + (int)pid_get_command(&main_rotor);

    if (pwm > PWM_MAX) {
        pwm = PWM_MAX;
    } else if (pwm < PWM_MIN) {
        pwm = PWM_MIN;
    }
    // printing out raw PID control value

    raw = pid_get_command(&main_rotor);
    xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
    UARTprintf("PWM = %d\n", raw);
    xSemaphoreGive(g_pUARTSemaphore);

    //pid_update(&tail, yaw_error, CONTROL_DT);
    setPWM (PWM_START_RATE_HZ, pwm);
}
