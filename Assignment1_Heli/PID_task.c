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
#include "button_task.h"

#define CONTROL_DT (1.f/50.f)

PID main_rotor;
//PID tail;

extern xSemaphoreHandle g_pUARTSemaphore;
extern xQueueHandle g_pALTQueue;
extern xQueueHandle g_pTARGETQueue;


void PID_Task(void *pvParameters)
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

struct PID {
    int16_t accumulator;
    int16_t limit;
    int16_t error;
    int16_t derivative;
    uint8_t kp, ki, kd;
};

// Initialises the PID controller
void pid_init(PID* pid, uint8_t kp, uint8_t ki, uint8_t kd, int16_t accum_limit) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->accumulator = 0;
    pid->error = 0;
    pid->derivative = 0;
    pid->limit = accum_limit;

}

void pid_update(PID* pid, int16_t error, int16_t dt) {
    // Add the error to the integral
    pid->accumulator += error*dt;

    // Clamp the accumulator to the specified limit
    if (pid->accumulator > pid->limit) pid->accumulator = pid->limit;
    if (pid->accumulator < pid->limit) pid->accumulator = -pid->limit;

    // Calculate the derivative
    pid->derivative = (error - pid->error)/ dt;
    // Store the current error
    pid->error = error;
}

float pid_get_command(PID* pid) {
    return pid->kp * pid->error + pid->ki * pid->accumulator + pid->kd * pid->derivative;
}

void control_init(void) {
    pid_init(&main_rotor, 1.0, 0, 1.0, 100.0);
    //pid_init(&tail, 0.2f, 2.0f, 0.0f, 300.0f);
}

void control_update(int16_t alt_error, int16_t yaw_error) {
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
