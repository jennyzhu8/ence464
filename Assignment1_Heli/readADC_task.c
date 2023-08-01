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
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "readADC_task.h"


//*****************************************************************************
//
// This task reads the ADC inputs from the pins for height/yaw
//
//*****************************************************************************
void ADC_task(void)
{
    while(1)
    {

    }
}
