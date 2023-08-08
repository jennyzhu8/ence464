/*
 * PID_task.c
 *
 *  Created on: 9/08/2023
 *      Author: bth74
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
#include "PID_task.h"
#include "PWM.h"

#define DT (1.f/50.f)

extern xQueueHandle g_pPWMQueue;
extern xSemaphoreHandle g_pUARTSemaphore;

void PID_Task(void *pvParameters)
{
    uint8_t i8Message;
    portTickType ui16LastTime;
    uint32_t ui32SwitchDelay = 25;
    uint8_t target_height = 50;
    uint8_t height;
    uint32_t pwm;
    uint32_t height_error;
    uint32_t prev_error = 0;
    uint32_t integral;
    uint32_t proportional;
    while(1)
    {
        if(xQueueReceive(g_pPWMQueue, &i8Message, pdMS_TO_TICKS(125)) == pdPASS) // ticks to wait must be > 0 so the task doesn't get stuck here
        {
            xSemaphoreTake(g_pUARTSemaphore, portMAX_DELAY);
            UARTprintf("Altitude = %d\n", i8Message);
            xSemaphoreGive(g_pUARTSemaphore);
            vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);

            height = i8Message;
            height_error = target_height - height;
            integral = (height_error - prev_error)*DT;
            proportional = (height_error);
            prev_error = height_error;
            pwm = 53 + proportional;
            if (pwm > 80)
            {
                pwm = 80;
            }
            setPWM (PWM_START_RATE_HZ, pwm);
        }

    }
}
