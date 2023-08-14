/*
 * Group 21
 *
 * Harrison Johhson
 *
 * Read ADC task, gets data from emulator
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
#include "semphr.h"
#include "readADC_task.h"


//*****************************************************************************
//
// This task reads the ADC inputs from the pins for height/yaw
//
//*****************************************************************************
extern xQueueHandle g_pALTQueue;

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

void altitude_ADC_task(void *pvParameters)
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
    uint8_t ui8message;
    uint32_t max_voltage;
    uint32_t min_voltage;
    uint32_t voltage_difference = 1348;

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
    // Setting initial altitude values
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

        height = alt-min_voltage;
        //getting height as a percentage
        height =  (100 - ((height*100)/voltage_difference));
        ui8message = height;

        if(xQueueSend(g_pALTQueue, &ui8message, portMAX_DELAY) != pdPASS)
        {
            // Error. The queue should never be full. If so print the
            // error message on UART and wait for ever.
            UARTprintf("\nQueue full. This should never happen.\n");
            while(1)
            {
            }
        }
        vTaskDelayUntil(&ui16LastTime, ui32SwitchDelay / portTICK_RATE_MS);
    }
}

